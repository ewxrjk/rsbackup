// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "Utils.h"
#include "ConfDirective.h"
#include <glob.h>

// ConfDirective --------------------------------------------------------------

ConfDirective::ConfDirective(const char *name_, int min_, int max_):
  name(name_), min(min_), max(max_) {
  if(!directives)
    directives = new directives_type();
  assert((*directives).find(name) == (*directives).end());
  (*directives)[name] = this;
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
}

bool ConfDirective::get_boolean(const ConfContext &cc) const {
  if(cc.bits.size() == 1) {
    warning("%s:%d: use '%s true' instead of '%s'",
            cc.path.c_str(), cc.line,
            name.c_str(), name.c_str());
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

// HostOnlyDirective ----------------------------------------------------------

void HostOnlyDirective::check(const ConfContext &cc) const {
  if(cc.host == nullptr)
    throw SyntaxError("'" + name + "' command without 'host'");
  if(cc.volume != nullptr && !inheritable)
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
    if(cc.bits[1] == "rgb"
       || cc.bits[1] == "hsv")
      ;                               // OK
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
  set(cc, Color(parseFloat(cc.bits[n], 0, 1),
                parseFloat(cc.bits[n+1], 0, 1),
                parseFloat(cc.bits[n+2], 0, 1)));
}

void ColorDirective::set_hsv(ConfContext &cc, size_t n) const {
  set(cc, Color::HSV(parseFloat(cc.bits[n]),
                     parseFloat(cc.bits[n+1], 0, 1),
                     parseFloat(cc.bits[n+2], 0, 1)));
}

void ColorDirective::set_packed(ConfContext &cc, size_t n, int radix) const {
  set(cc, Color(parseInteger(cc.bits[n], 0, 0xFFFFFF, radix)));
}

// Global directives ----------------------------------------------------------

/** @brief The @c store directive */
static const struct StoreDirective: public ConfDirective {
  StoreDirective(): ConfDirective("store", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->stores[cc.bits[1]] = new Store(cc.bits[1]);
  }
} store_directive;

/** @brief The @c store-pattern directive */
static const struct StorePatternDirective: public ConfDirective {
  StorePatternDirective(): ConfDirective("store-pattern", 1, 1) {}
  void set(ConfContext &cc) const override {
    std::vector<std::string> files;
    globFiles(files, cc.bits[1], GLOB_NOCHECK);
    for(auto &file: files)
      cc.conf->stores[file] = new Store(file);
  }
} store_pattern_directive;

/** @brief The @c stylesheet directive */
static const struct StyleSheetDirective: public ConfDirective {
  StyleSheetDirective(): ConfDirective("stylesheet", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->stylesheet = cc.bits[1];
  }
} stylesheet_directive;

/** @brief The @c colors directive */
static const struct ColorsDirective: public ConfDirective {
  ColorsDirective(): ConfDirective("colors", 2, 2) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'colors' directive is deprecated, use 'color-good' and 'color-bad' instead",
            cc.path.c_str(), cc.line);
    cc.conf->colorGood = parseInteger(cc.bits[1], 0, 0xFFFFFF, 0);
    cc.conf->colorBad = parseInteger(cc.bits[2], 0, 0xFFFFFF, 0);
  }
} colors_directive;

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
  PublicDirective(): ConfDirective("public", 0, 1) {}
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

/** @brief The @c pre-access-hook directive */
static const struct PreAccessHookDirective: public ConfDirective {
  PreAccessHookDirective(): ConfDirective("pre-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->preAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_access_hook_directive;

/** @brief The @c post-access-hook directive */
static const struct PostAccessHookDirective: public ConfDirective {
  PostAccessHookDirective(): ConfDirective("post-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->postAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_access_hook_directive;

/** @brief The @c keep-prune-logs directive */
static const struct KeepPruneLogsDirective: public ConfDirective {
  KeepPruneLogsDirective(): ConfDirective("keep-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->keepPruneLogs = parseInteger(cc.bits[1], 1);
  }
} keep_prune_logs_directive;

/** @brief The @c report-prune-logs directive */
static const struct ReportPruneLogsDirective: public ConfDirective {
  ReportPruneLogsDirective(): ConfDirective("report-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'report-prune-logs' directive is deprecated, use 'report' instead",
            cc.path.c_str(), cc.line);
    cc.conf->reportPruneLogs = parseInteger(cc.bits[1], 1);
  }
} report_prune_logs_directive;

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
  DeviceColorStrategyDirective(): ConfDirective("device-color-strategy",
                                            1, INT_MAX) {}
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
    cc.conf->horizontalPadding = parseFloat(cc.bits[1],
                                            0,
                                            std::numeric_limits<double>::max());
  }
} horizontal_padding_directive;

/** @brief The vertical-padding directive */
static const struct VerticalPaddingDirective: public ConfDirective {
  VerticalPaddingDirective(): ConfDirective("vertical-padding") {}
  void set(ConfContext &cc) const override {
    cc.conf->verticalPadding = parseFloat(cc.bits[1],
                                          0,
                                          std::numeric_limits<double>::max());
  }
} vertical_padding_directive;

/** @brief The backup-indicator-width directive */
static const struct BackupIndicatorWidthDirective: public ConfDirective {
  BackupIndicatorWidthDirective(): ConfDirective("backup-indicator-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorWidth
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_width_directive;

/** @brief The backup-indicator-height directive */
static const struct BackupIndicatorHeightDirective: public ConfDirective {
  BackupIndicatorHeightDirective(): ConfDirective("backup-indicator-height") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorHeight
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_height_directive;

/** @brief The backup-indicator-key-width directive */
static const struct BackupIndicatorKeyWidthDirective: public ConfDirective {
  BackupIndicatorKeyWidthDirective(): ConfDirective("backup-indicator-key-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorKeyWidth
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
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
static const struct MaxAgeDirective: public ConfDirective {
  MaxAgeDirective(): ConfDirective("max-age", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->maxAge = parseInteger(cc.bits[1], 1);
  }
} max_age_directive;

/** @brief The @c min-backups directive */
static const struct MinBackupsDirective: public ConfDirective {
  MinBackupsDirective(): ConfDirective("min-backups", 1, 1) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'min-backups' directive is deprecated, use 'prune-parameter min-backups' instead",
            cc.path.c_str(), cc.line);
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["min-backups"] = cc.bits[1];
  }
} min_backups_directive;

/** @brief The @c prune-age directive */
static const struct PruneAgeDirective: public ConfDirective {
  PruneAgeDirective(): ConfDirective("prune-age", 1, 1) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'prune-age' directive is deprecated, use 'prune-parameter prune-age' instead",
            cc.path.c_str(), cc.line);
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["prune-age"] = cc.bits[1];
  }
} prune_age_directive;

/** @brief The @c prune-policy directive */
static const struct PrunePolicyDirective: public ConfDirective {
  PrunePolicyDirective(): ConfDirective("prune-policy", 1, 1) {}
  void set(ConfContext &cc) const override {
    if(cc.bits[1].size() > 0 && cc.bits[1].at(0) == '/') {
      cc.context->prunePolicy = "exec";
      cc.context->pruneParameters["path"] = cc.bits[1];
    } else
      cc.context->prunePolicy = cc.bits[1];
  }
} prune_policy_directive;

/** @brief The @c prune-parameter directive */
static const struct PruneParameterDirective: public ConfDirective {
  PruneParameterDirective(): ConfDirective("prune-parameter", 2, 2) {}
  void check(const ConfContext &cc) const override {
    ConfDirective::check(cc);
    const std::string &name = (cc.bits[1] != "--remove" ?
                                             cc.bits[1] : cc.bits[2]);
    if(!valid(name))
      throw SyntaxError("invalid prune-parameter name");
  }
  void set(ConfContext &cc) const override {
    if(cc.bits[1] != "--remove")
      cc.context->pruneParameters[cc.bits[1]] = cc.bits[2];
    else
      cc.context->pruneParameters.erase(cc.bits[2]);
  }
  static bool valid(const std::string &name) {
    return name.size() > 0
      && name.at(0) != '-'
      && name.find_first_not_of(PRUNE_PARAMETER_VALID) == std::string::npos;
  }
} prune_parameter_directive;

/** @brief The @c pre-backup-hook directive */
static const struct PreBackupHookDirective: public ConfDirective {
  PreBackupHookDirective(): ConfDirective("pre-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->preBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_backup_hook_directive;

/** @brief The @c post-backup-hook directive */
static const struct PostBackupHookDirective: public ConfDirective {
  PostBackupHookDirective(): ConfDirective("post-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->postBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_backup_hook_directive;

/** @brief The @c rsync-timeout directive */
static const struct RsyncTimeoutDirective: public ConfDirective {
  RsyncTimeoutDirective(): ConfDirective("rsync-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncTimeout = parseInteger(cc.bits[1], 1);
  }
} rsync_timeout_directive;

/** @brief The @c hook-timeout directive */
static const struct HookTimeoutDirective: public ConfDirective {
  HookTimeoutDirective(): ConfDirective("hook-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->hookTimeout = parseInteger(cc.bits[1], 1);
  }
} hook_timeout_directive;

/** @brief The @c ssh-timeout directive */
static const struct SshTimeoutDirective: public ConfDirective {
  SshTimeoutDirective(): ConfDirective("ssh-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->sshTimeout = parseInteger(cc.bits[1], 1);
  }
} ssh_timeout_directive;

// Host directives ------------------------------------------------------------

/** @brief The @c host directive */
static const struct HostDirective: public ConfDirective {
  HostDirective(): ConfDirective("host", 1, 1) {}
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

/** @brief The @c always-up directive */
static const struct AlwaysUpDirective: public HostOnlyDirective {
  AlwaysUpDirective(): HostOnlyDirective("always-up", 0, 1) {}
  void set(ConfContext &cc) const override {
    cc.host->alwaysUp = get_boolean(cc);
  }
} always_up_directive;

/** @brief The @c priority directive */
static const struct PriorityDirective: public HostOnlyDirective {
  PriorityDirective(): HostOnlyDirective("priority", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.host->priority = parseInteger(cc.bits[1]);
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
  DevicesDirective(): HostOnlyDirective("devices", 1, 1, true) {}
  void set(ConfContext &cc) const override {
    cc.context->devicePattern = cc.bits[1];
  }
} devices_directive;

// Volume directives ----------------------------------------------------------

/** @brief The @c volume directive */
static const struct VolumeDirective: public HostOnlyDirective {
  VolumeDirective(): HostOnlyDirective("volume", 2, 2, true/*hacky*/) {}
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
  CheckMountedDirective(): VolumeOnlyDirective("check-mounted", 0, 1) {}
  void set(ConfContext &cc) const override {
    cc.volume->checkMounted = get_boolean(cc);
  }
} check_mounted_directive;
