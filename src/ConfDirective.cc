// Copyright © 2011, 2012, 2014-20 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <config.h>
#include "rsbackup.h"
#include "Location.h"
#include "Conf.h"
#include "Device.h"
#include "Backup.h"
#include "Volume.h"
#include "Host.h"
#include "Store.h"
#include "Errors.h"
#include "Utils.h"
#include "ConfDirective.h"
#include <glob.h>

// ConfDirective --------------------------------------------------------------

ConfDirective::ConfDirective(const char *name_, int min_, int max_,
                             unsigned acceptable_levels_, unsigned new_level_):
    name(name_),
    acceptable_levels(acceptable_levels_), new_level(new_level_), min(min_),
    max(max_) {
  if(!directives)
    directives = new directives_type();
  if(!aliases)
    aliases = new std::set<std::string>();
  assert((*directives).find(name) == (*directives).end());
  (*directives)[name] = this;
}

void ConfDirective::alias(const char *name_) {
  std::string n(name_);
  assert(directives != nullptr);
  assert((*directives).find(n) == (*directives).end());
  (*directives)[n] = this;
  assert(aliases != nullptr);
  aliases->insert(n);
}

const ConfDirective *ConfDirective::find(const std::string &name) {
  auto it = directives->find(name);
  return it == directives->end() ? nullptr : it->second;
}

void ConfDirective::check(const ConfContext &cc) const {
  int args = cc.bits.size() - 1;
  if(args < min)
    throw SyntaxError("too few arguments to '" + name + "'");
  if(args > max)
    throw SyntaxError("too many arguments to '" + name + "'");
  if(aliases->find(cc.bits[0]) != aliases->end())
    warning(WARNING_DEPRECATED,
            "%s:%d: the '%s' directive is deprecated, use '%s' instead",
            cc.location.path.c_str(), cc.location.line, cc.bits[0].c_str(),
            name.c_str());
}

bool ConfDirective::get_boolean(const ConfContext &cc) const {
  if(cc.bits.size() == 1) {
    warning(WARNING_DEPRECATED, "%s:%d: use '%s true' instead of '%s'",
            cc.location.path.c_str(), cc.location.line, name.c_str(),
            name.c_str());
    return true;
  } else if(cc.bits[1] == "true")
    return true;
  else if(cc.bits[1] == "false")
    return false;
  else
    throw SyntaxError("invalid argument to '" + name
                      + "' - only 'true' or 'false' allowed");
}

void ConfDirective::extend(const ConfContext &cc,
                           std::vector<std::string> &conf) const {
  if(cc.bits[1] == "+")
    conf.insert(conf.end(), &cc.bits[2], &cc.bits[cc.bits.size()]);
  else
    conf.assign(&cc.bits[1], &cc.bits[cc.bits.size()]);
}

directives_type *ConfDirective::directives;
std::set<std::string> *ConfDirective::aliases;

// HostOnlyDirective ----------------------------------------------------------

void HostOnlyDirective::check(const ConfContext &cc) const {
  if(cc.host == nullptr)
    throw SyntaxError("'" + name + "' command without 'host'");
  if(cc.volume != nullptr && !(acceptable_levels & LEVEL_VOLUME))
    throw SyntaxError("'" + name + "' inside 'volume'");
  ConfDirective::check(cc);
}

// VolumeOnlyDirective --------------------------------------------------------

void VolumeOnlyDirective::check(const ConfContext &cc) const {
  if(cc.volume == nullptr)
    throw SyntaxError("'" + name + "' command without 'volume'");
  ConfDirective::check(cc);
}

// ColorDirective -------------------------------------------------------------

void ColorDirective::check(const ConfContext &cc) const {
  int args = cc.bits.size() - 1;
  ConfDirective::check(cc);
  if(args > 1 && args < 4)
    throw SyntaxError("wrong number of arguments to '" + name + "'");
  if(args == 4) {
    if(cc.bits[1] == "rgb" || cc.bits[1] == "hsv")
      ; // OK
    else
      throw SyntaxError("invalid color representation '" + cc.bits[1] + "'");
  }
}

void ColorDirective::set(ConfContext &cc) const {
  int args = cc.bits.size() - 1;
  if(args == 4) {
    if(cc.bits[1] == "rgb")
      set_rgb(cc, 2);
    else if(cc.bits[1] == "hsv")
      set_hsv(cc, 2);
  } else
    set_packed(cc, 1, 0);
}

void ColorDirective::set_rgb(ConfContext &cc, size_t n) const {
  set(cc, Color(parseFloat(cc.bits[n], 0, 1), parseFloat(cc.bits[n + 1], 0, 1),
                parseFloat(cc.bits[n + 2], 0, 1)));
}

void ColorDirective::set_hsv(ConfContext &cc, size_t n) const {
  set(cc, Color::HSV(parseFloat(cc.bits[n]), parseFloat(cc.bits[n + 1], 0, 1),
                     parseFloat(cc.bits[n + 2], 0, 1)));
}

void ColorDirective::set_packed(ConfContext &cc, size_t n, int radix) const {
  set(cc, Color(parseInteger(cc.bits[n], 0, 0xFFFFFF, radix)));
}

// Global directives ----------------------------------------------------------

/** @brief Parse arguments to the @c store directive
 * @param cc Configuration context
 * @param mounted Set to whether the store must be mounted
 * @return Index of the argument after the options
 */
size_t parseStoreArguments(const ConfContext &cc, bool &mounted) {
  mounted = true;
  size_t i = 1;
  while(i < cc.bits.size() && cc.bits[i][0] == '-') {
    if(cc.bits[i] == "--mounted")
      mounted = true;
    else if(cc.bits[i] == "--no-mounted")
      mounted = false;
    else
      throw SyntaxError("unrecognized store option");
    ++i;
  }
  if(i >= cc.bits.size())
    throw SyntaxError("missing store path");
  return i;
}

/** @brief The @c store directive */
static const struct StoreDirective: public ConfDirective {
  StoreDirective(): ConfDirective("store", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    bool mounted;
    size_t i = parseStoreArguments(cc, mounted);
    cc.conf->stores[cc.bits[i]] = new Store(cc.bits[i], mounted);
  }
} store_directive;

/** @brief The @c store-pattern directive */
static const struct StorePatternDirective: public ConfDirective {
  StorePatternDirective(): ConfDirective("store-pattern", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    std::vector<std::string> files;
    bool mounted;
    size_t i = parseStoreArguments(cc, mounted);
    globFiles(files, cc.bits[i], GLOB_NOCHECK);
    for(auto &file: files)
      cc.conf->stores[file] = new Store(file, mounted);
  }
} store_pattern_directive;

/** @brief The @c stylesheet directive */
static const struct StyleSheetDirective: public ConfDirective {
  StyleSheetDirective(): ConfDirective("stylesheet", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->stylesheet = cc.bits[1];
  }
} stylesheet_directive;

/** @brief The @c color-good directive */
static const struct ColorGoodDirective: public ColorDirective {
  ColorGoodDirective(): ColorDirective("color-good") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorGood = c;
  }
} color_good_directive;

/** @brief The @c color-bad directive */
static const struct ColorBadDirective: public ColorDirective {
  ColorBadDirective(): ColorDirective("color-bad") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorBad = c;
  }
} color_bad_directive;

/** @brief The @c device directive */
static const struct DeviceDirective: public ConfDirective {
  DeviceDirective(): ConfDirective("device", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->devices[cc.bits[1]] = new Device(cc.bits[1]);
  }
} device_directive;

/** @brief The @c max-usage directive */
static const struct MaxUsageDirective: public ConfDirective {
  MaxUsageDirective(): ConfDirective("max-usage", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->maxUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_usage_directive;

/** @brief The @c max-file-usage directive */
static const struct MaxFileUsageDirective: public ConfDirective {
  MaxFileUsageDirective(): ConfDirective("max-file-usage", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->maxFileUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_file_usage_directive;

/** @brief The @c public directive */
static const struct PublicDirective: public ConfDirective {
  PublicDirective(): ConfDirective("public", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->publicStores = get_boolean(cc);
  }
} public_directive;

/** @brief The @c logs directive */
static const struct LogsDirective: public ConfDirective {
  LogsDirective(): ConfDirective("logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->logs = cc.bits[1];
  }
} logs_directive;

/** @brief The @c database directive */
static const struct DatabaseDirective: public ConfDirective {
  DatabaseDirective(): ConfDirective("database", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->database = cc.bits[1];
  }
} database_directive;

/** @brief The @c lock directive */
static const struct LockDirective: public ConfDirective {
  LockDirective(): ConfDirective("lock", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->lock = cc.bits[1];
  }
} lock_directive;

/** @brief The @c sendmail directive */
static const struct SendmailDirective: public ConfDirective {
  SendmailDirective(): ConfDirective("sendmail", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->sendmail = cc.bits[1];
  }
} sendmail_directive;

/** @brief The @c rm directive */
static const struct RmDirective: public ConfDirective {
  RmDirective(): ConfDirective("rm", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->rm = cc.bits[1];
  }
} rm_directive;

/** @brief The @c pre-device-hook directive */
static const struct PreDeviceHookDirective: public ConfDirective {
  PreDeviceHookDirective(): ConfDirective("pre-device-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->preDevice.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_device_hook_directive;

/** @brief The @c post-device-hook directive */
static const struct PostDeviceHookDirective: public ConfDirective {
  PostDeviceHookDirective(): ConfDirective("post-device-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->postDevice.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} device;

/** @brief The @c keep-prune-logs directive */
static const struct KeepPruneLogsDirective: public ConfDirective {
  KeepPruneLogsDirective(): ConfDirective("keep-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->keepPruneLogs = parseTimeInterval(cc.bits[1]);
  }
} keep_prune_logs_directive;

/** @brief The @c prune-timeout directive */
static const struct PruneTimeoutDirective: public ConfDirective {
  PruneTimeoutDirective(): ConfDirective("prune-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->pruneTimeout = parseTimeInterval(cc.bits[1]);
  }
} prune_timeout_directive;

/** @brief The @c include directive */
static const struct IncludeDirective: public ConfDirective {
  IncludeDirective(): ConfDirective("include", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->includeFile(cc.bits[1]);
  }
} include_directive;

/** @brief The color-graph-background directive */
static const struct ColorGraphBackgroundDirective: public ColorDirective {
  ColorGraphBackgroundDirective(): ColorDirective("color-graph-background") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorGraphBackground = c;
  }
} color_graph_background_directive;

/** @brief The color-graph-foreground directive */
static const struct ColorGraphForegroundDirective: public ColorDirective {
  ColorGraphForegroundDirective(): ColorDirective("color-graph-foreground") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorGraphForeground = c;
  }
} color_graph_foreground_directive;

/** @brief The color-month-guide directive */
static const struct ColorMonthGuideDirective: public ColorDirective {
  ColorMonthGuideDirective(): ColorDirective("color-month-guide") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorMonthGuide = c;
  }
} color_month_guide_directive;

/** @brief The color-host-guide directive */
static const struct ColorHostGuideDirective: public ColorDirective {
  ColorHostGuideDirective(): ColorDirective("color-host-guide") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorHostGuide = c;
  }
} color_host_guide_directive;

/** @brief The color-volume-guide directive */
static const struct ColorVolumeGuideDirective: public ColorDirective {
  ColorVolumeGuideDirective(): ColorDirective("color-volume-guide") {}
  void set(ConfContext &cc, const Color &c) const override {
    cc.conf->colorVolumeGuide = c;
  }
} color_volume_guide_directive;

/** @brief The device-color-strategy directive */
static const struct DeviceColorStrategyDirective: public ConfDirective {
  DeviceColorStrategyDirective():
      ConfDirective("device-color-strategy", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    ColorStrategy *nc = ColorStrategy::create(cc.bits[1], cc.bits, 2);
    delete cc.conf->deviceColorStrategy;
    cc.conf->deviceColorStrategy = nc;
  }
} device_color_strategy_directive;

/** @brief The horizontal-padding directive */
static const struct HorizontalPaddingDirective: public ConfDirective {
  HorizontalPaddingDirective(): ConfDirective("horizontal-padding") {}
  void set(ConfContext &cc) const override {
    cc.conf->horizontalPadding =
        parseFloat(cc.bits[1], 0, std::numeric_limits<double>::max());
  }
} horizontal_padding_directive;

/** @brief The vertical-padding directive */
static const struct VerticalPaddingDirective: public ConfDirective {
  VerticalPaddingDirective(): ConfDirective("vertical-padding") {}
  void set(ConfContext &cc) const override {
    cc.conf->verticalPadding =
        parseFloat(cc.bits[1], 0, std::numeric_limits<double>::max());
  }
} vertical_padding_directive;

/** @brief The backup-indicator-width directive */
static const struct BackupIndicatorWidthDirective: public ConfDirective {
  BackupIndicatorWidthDirective(): ConfDirective("backup-indicator-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorWidth =
        parseFloat(cc.bits[1], std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_width_directive;

/** @brief The backup-indicator-height directive */
static const struct BackupIndicatorHeightDirective: public ConfDirective {
  BackupIndicatorHeightDirective(): ConfDirective("backup-indicator-height") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorHeight =
        parseFloat(cc.bits[1], std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_height_directive;

/** @brief The graph-target-width directive */
static const struct GraphTargetWidthDirective: public ConfDirective {
  GraphTargetWidthDirective(): ConfDirective("graph-target-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->graphTargetWidth =
        parseFloat(cc.bits[1], 0, std::numeric_limits<double>::max());
  }
} graph_target_width_directive;

/** @brief The backup-indicator-key-width directive */
static const struct BackupIndicatorKeyWidthDirective: public ConfDirective {
  BackupIndicatorKeyWidthDirective():
      ConfDirective("backup-indicator-key-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorKeyWidth =
        parseFloat(cc.bits[1], std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_key_width_directive;

/** @brief The host-name-font directive */
static const struct HostNameFontDirective: public ConfDirective {
  HostNameFontDirective(): ConfDirective("host-name-font", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->hostNameFont = cc.bits[1];
  }
} host_name_font_directive;

/** @brief The volume-name-font directive */
static const struct VolumeNameFontDirective: public ConfDirective {
  VolumeNameFontDirective(): ConfDirective("volume-name-font", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->volumeNameFont = cc.bits[1];
  }
} volume_name_font_directive;

/** @brief The device-name-font directive */
static const struct DeviceNameFontDirective: public ConfDirective {
  DeviceNameFontDirective(): ConfDirective("device-name-font", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->deviceNameFont = cc.bits[1];
  }
} device_name_font_directive;

/** @brief The time-label-font directive */
static const struct TimeLabelFontDirective: public ConfDirective {
  TimeLabelFontDirective(): ConfDirective("time-label-font", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->timeLabelFont = cc.bits[1];
  }
} time_label_font_directive;

/** @brief The report directive */
static const struct ReportDirective: public ConfDirective {
  ReportDirective(): ConfDirective("report", 0, INT_MAX) {}
  void set(ConfContext &cc) const override {
    extend(cc, cc.conf->report);
  }
} report_directive;

/** @brief The graph-layout directive */
static const struct GraphLayoutDirective: public ConfDirective {
  GraphLayoutDirective(): ConfDirective("graph-layout", 0, INT_MAX) {}
  void set(ConfContext &cc) const override {
    extend(cc, cc.conf->graphLayout);
  }
} graph_layout_directive;

// Inheritable directives -----------------------------------------------------

/** @brief The @c max-age directive */
static const struct MaxAgeDirective: InheritableDirective {
  MaxAgeDirective(): InheritableDirective("max-age", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->maxAge = parseTimeInterval(cc.bits[1]);
  }
} max_age_directive;

/** @brief The @c backup-policy directive */
static const struct BackupPolicyDirective: InheritableDirective {
  BackupPolicyDirective(): InheritableDirective("backup-policy", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->backupPolicy = cc.bits[1];
  }
} backup_policy_directive;

/** @brief The @c backup-parameter directive */
static const struct BackupParameterDirective: InheritableDirective {
  BackupParameterDirective(): InheritableDirective("backup-parameter", 2, 2) {}
  void check(const ConfContext &cc) const override {
    ConfDirective::check(cc);
    const std::string &name =
        (cc.bits[1] != "--remove" ? cc.bits[1] : cc.bits[2]);
    if(!valid(name))
      throw SyntaxError("invalid backup-parameter name");
  }
  void set(ConfContext &cc) const override {
    if(cc.bits[1] != "--remove")
      cc.context->backupParameters[cc.bits[1]] =
          PolicyParameter(cc.bits[2], cc.location);
    else
      cc.context->backupParameters.erase(cc.bits[2]);
  }

  /** @brief Test for valid @c backup-parameter names
   * @param name Candidate name
   * @return @c true if @name is valid
   */
  static bool valid(const std::string &name) {
    return name.size() > 0 && name.at(0) != '-'
           && name.find_first_not_of(POLICY_PARAMETER_VALID)
                  == std::string::npos;
  }
} backup_parameter_directive;

/** @brief The @c prune-policy directive */
static const struct PrunePolicyDirective: InheritableDirective {
  PrunePolicyDirective(): InheritableDirective("prune-policy", 1, 1) {}
  void set(ConfContext &cc) const override {
    if(cc.bits[1].size() > 0 && cc.bits[1].at(0) == '/') {
      cc.context->prunePolicy = "exec";
      cc.context->pruneParameters["path"] =
          PolicyParameter(cc.bits[1], cc.location);
    } else
      cc.context->prunePolicy = cc.bits[1];
  }
} prune_policy_directive;

/** @brief The @c prune-parameter directive */
static const struct PruneParameterDirective: InheritableDirective {
  PruneParameterDirective(): InheritableDirective("prune-parameter", 2, 2) {}
  void check(const ConfContext &cc) const override {
    ConfDirective::check(cc);
    const std::string &name =
        (cc.bits[1] != "--remove" ? cc.bits[1] : cc.bits[2]);
    if(!valid(name))
      throw SyntaxError("invalid prune-parameter name");
  }
  void set(ConfContext &cc) const override {
    if(cc.bits[1] != "--remove")
      cc.context->pruneParameters[cc.bits[1]] =
          PolicyParameter(cc.bits[2], cc.location);
    else
      cc.context->pruneParameters.erase(cc.bits[2]);
  }

  /** @brief Test for valid @c prune-parameter names
   * @param name Candidate name
   * @return @c true if @name is valid
   */
  static bool valid(const std::string &name) {
    return name.size() > 0 && name.at(0) != '-'
           && name.find_first_not_of(POLICY_PARAMETER_VALID)
                  == std::string::npos;
  }
} prune_parameter_directive;

/** @brief The @c pre-volume-hook directive */
static const struct PreVolumeHookDirective: InheritableDirective {
  PreVolumeHookDirective():
      InheritableDirective("pre-volume-hook", 0, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->preVolume.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_volume_hook_directive;

/** @brief The @c post-volume-hook directive */
static const struct PostVolumeHookDirective: InheritableDirective {
  PostVolumeHookDirective():
      InheritableDirective("post-volume-hook", 0, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->postVolume.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_volume_hook_directive;

/** @brief The @c backup-job-timeout directive */
static const struct BackupJobTimeoutDirective: InheritableDirective {
  BackupJobTimeoutDirective():
      InheritableDirective("backup-job-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->backupJobTimeout = parseTimeInterval(cc.bits[1]);
  }
} backup_job_directive;

/** @brief The @c rsync-io-timeout directive */
static const struct RsyncIOTimeoutDirective: InheritableDirective {
  RsyncIOTimeoutDirective(): InheritableDirective("rsync-io-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncIOTimeout = parseTimeInterval(cc.bits[1]);
  }
} rsync_io_timeout_directive;

/** @brief The @c hook-timeout directive */
static const struct HookTimeoutDirective: InheritableDirective {
  HookTimeoutDirective(): InheritableDirective("hook-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->hookTimeout = parseTimeInterval(cc.bits[1]);
  }
} hook_timeout_directive;

/** @brief The @c host-check directive */
static const struct HostCheckDirective: InheritableDirective {
  HostCheckDirective():
      InheritableDirective("host-check", 1, INT_MAX, LEVEL_TOP | LEVEL_HOST) {}
  void set(ConfContext &cc) const override {
    if(cc.bits[1] == "ssh" || cc.bits[1] == "always-up") {
      if(cc.bits.size() != 2)
        throw SyntaxError("invalid host-check syntax");
    } else if(cc.bits[1] == "command") {
      // OK
    } else {
      throw SyntaxError("unrecognized host-check type");
    }
    cc.context->hostCheck.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} host_check_directive;

/** @brief The @c ssh-timeout directive */
static const struct SshTimeoutDirective: InheritableDirective {
  SshTimeoutDirective(): InheritableDirective("ssh-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->sshTimeout = parseTimeInterval(cc.bits[1]);
  }
} ssh_timeout_directive;

/** @brief The @c rsync-command directive */
static const struct RsyncCommandDirective: InheritableDirective {
  RsyncCommandDirective(): InheritableDirective("rsync-command", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncCommand = cc.bits[1];
  }
} rsync_command_directive;

/** @brief The @c rsync-link-dest directive */
static const struct RsyncLinkDestDirective: InheritableDirective {
  RsyncLinkDestDirective(): InheritableDirective("rsync-link-dest", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncLinkDest = get_boolean(cc);
  }
} rsync_link_dest_directive;

/** @brief The @c rsync-base-options directive */
static const struct RsyncBaseOptionsDirective: InheritableDirective {
  RsyncBaseOptionsDirective():
      InheritableDirective("rsync-base-options", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncBaseOptions.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} rsync_base_options_directive;

/** @brief The @c rsync-extra-options directive */
static const struct RsyncExtraOptionsDirective: InheritableDirective {
  RsyncExtraOptionsDirective():
      InheritableDirective("rsync-extra-options", 0, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncExtraOptions.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} rsync_extra_options_directive;

/** @brief The @c rsync-remote directive */
static const struct RsyncRemoteDirective: InheritableDirective {
  RsyncRemoteDirective(): InheritableDirective("rsync-remote", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncRemote = cc.bits[1];
  }
} rsync_remote_directive;

static const struct BackupTimeDirective: InheritableDirective {
  BackupTimeDirective(): InheritableDirective("backup-time", 1, 1) {}
  void set(ConfContext &cc) const override {
    // Currently we allow just a single time range. The syntax is chosen
    // so that adding additional time ranges shouldn't have any compatibility
    // issues.
    const size_t dash = cc.bits[1].find('-');
    if(dash == std::string::npos)
      throw SyntaxError("expected EARLIEST-LATEST");

    int earliest = parseTimeOfDay(cc.bits[1].substr(0, dash));
    int latest = parseTimeOfDay(cc.bits[1].substr(dash + 1, std::string::npos));

    if(latest == 0)
      latest = 86400;
    if(earliest > latest)
      throw SyntaxError("earliest backup time (" + formatTimeOfDay(earliest)
                        + ") is later than latest backup time ("
                        + formatTimeOfDay(latest) + ")");
    cc.context->earliest = earliest;
    cc.context->latest = latest;
  }
} backup_time_directive;

/** @brief The @c group directive */
static const struct GroupDirective: public InheritableDirective {
  GroupDirective(): InheritableDirective("group", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->group = cc.bits[1];
  }
} group_directive;

// Host directives ------------------------------------------------------------

/** @brief The @c host directive */
static const struct HostDirective: public ConfDirective {
  HostDirective(): ConfDirective("host", 1, 1, LEVEL_TOP, LEVEL_HOST) {}
  void set(ConfContext &cc) const override {
    if(!Host::valid(cc.bits[1]))
      throw SyntaxError("invalid host name");
    if(contains(cc.conf->hosts, cc.bits[1]))
      throw SyntaxError("duplicate host");
    cc.context = cc.host = new Host(cc.conf, cc.bits[1]);
    cc.volume = nullptr;
    cc.host->hostname = cc.bits[1];
  }
} host_directive;

/** @brief The @c hostname directive */
static const struct HostnameDirective: public HostOnlyDirective {
  HostnameDirective(): HostOnlyDirective("hostname", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.host->hostname = cc.bits[1];
  }
} hostname_directive;

/** @brief The @c priority directive */
static const struct PriorityDirective: public HostOnlyDirective {
  PriorityDirective(): HostOnlyDirective("priority", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.host->priority =
        parseInteger(cc.bits[1], std::numeric_limits<int>::min(),
                     std::numeric_limits<int>::max());
  }
} priority_directive;

/** @brief The @c user directive */
static const struct UserDirective: public HostOnlyDirective {
  UserDirective(): HostOnlyDirective("user", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.host->user = cc.bits[1];
  }
} user_directive;

/** @brief The @c devices directive */
static const struct DevicesDirective: public HostOnlyDirective {
  DevicesDirective():
      HostOnlyDirective("devices", 1, 1, LEVEL_HOST | LEVEL_VOLUME) {}
  void set(ConfContext &cc) const override {
    cc.context->devicePattern = cc.bits[1];
  }
} devices_directive;

// Volume directives ----------------------------------------------------------

/** @brief The @c volume directive */
static const struct VolumeDirective: public HostOnlyDirective {
  VolumeDirective():
      HostOnlyDirective("volume", 2, 2, LEVEL_HOST, LEVEL_VOLUME) {}
  void set(ConfContext &cc) const override {
    if(!Volume::valid(cc.bits[1]))
      throw SyntaxError("invalid volume name");
    if(contains(cc.host->volumes, cc.bits[1]))
      throw SyntaxError("duplicate volume");
    cc.context = cc.volume = new Volume(cc.host, cc.bits[1], cc.bits[2]);
  }
} volume_directive;

/** @brief The @c exclude directive */
static const struct ExcludeDirective: public VolumeOnlyDirective {
  ExcludeDirective(): VolumeOnlyDirective("exclude", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.volume->exclude.push_back(cc.bits[1]);
  }
} exclude_directive;

/** @brief The @c traverse directive */
static const struct TraverseDirective: public VolumeOnlyDirective {
  TraverseDirective(): VolumeOnlyDirective("traverse", 0, 1) {}
  void set(ConfContext &cc) const override {
    cc.volume->traverse = get_boolean(cc);
  }
} traverse_directive;

/** @brief The @c check-file directive */
static const struct CheckFileDirective: public VolumeOnlyDirective {
  CheckFileDirective(): VolumeOnlyDirective("check-file", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.volume->checkFile = cc.bits[1];
  }
} check_file_directive;

/** @brief The @c check-mounted directive */
static const struct CheckMountedDirective: public VolumeOnlyDirective {
  CheckMountedDirective(): VolumeOnlyDirective("check-mounted", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.volume->checkMounted = get_boolean(cc);
  }
} check_mounted_directive;
