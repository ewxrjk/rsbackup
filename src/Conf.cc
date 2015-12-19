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
#include "IO.h"
#include "Command.h"
#include "Utils.h"
#include "Database.h"
#include "Prune.h"
#include <cctype>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <glob.h>
#include <regex>
#include <boost/filesystem.hpp>

/** @brief Context for configuration file parsing */
struct ConfContext {
  /** @brief Constructor
   *
   * @param conf_ Root configuration node
   */
  ConfContext(Conf *conf_):
    conf(conf_), context(conf_) {}

  /** @brief Root of configuration */
  Conf *conf;

  /** @brief Current configuration node
   *
   * Could be a @ref Conf, @ref Host or @ref Volume.
   */
  ConfBase *context;

  /** @brief Current host or null */
  Host *host = nullptr;

  /** @brief Current volume or null */
  Volume *volume = nullptr;

  /** @brief Parsed directive */
  std::vector<std::string> bits;

  /** @brief Containing filename */
  std::string path;

  /** @brief Line number */
  int line = -1;
};

struct Directive;

/** @brief Type of name-to-directive map */
typedef std::map<std::string, const Directive *> directives_type;

/** @brief Map names to directives */
static directives_type *directives;

/** @brief Base class for configuration file directives */
struct Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   *
   * Directives are automatically registered in this constructor.
   */
  Directive(const char *name_, int min_=0, int max_=INT_MAX):
    name(name_), min(min_), max(max_) {
    if(!directives)
      directives = new directives_type();
    assert((*directives).find(name) == (*directives).end());
    (*directives)[name] = this;
  }

  /** @brief Name of directive */
  const std::string name;

  /** @brief Minimum number of arguments */
  int min;

  /** @brief Maximum number of arguments */
  int max;

  /** @brief Check directive syntax
   * @param cc Context containing directive
   *
   * The base class implementation just checks the minimum and maximum number
   * of arguments.
   */
  virtual void check(const ConfContext &cc) const {
    int args = cc.bits.size() - 1;
    if(args < min)
      throw SyntaxError("too few arguments to '" + name + "'");
    if(args > max)
      throw SyntaxError("too many arguments to '" + name + "'");
  }

  /** @brief Get a boolean parameter
   * @param cc Context containing directive
   * @return @c true or @c false
   *
   * Use in Directive::set implementations for boolean-sense directives.
   */
  bool get_boolean(const ConfContext &cc) const {
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

  /** @brief Act on a directive
   *  @param cc Context containing directive
   */
  virtual void set(ConfContext &cc) const = 0;
};

/** @brief Base class for directives that can only appear in a host context */
struct HostOnlyDirective: public Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   * @param inheritable_ If @c true, also allowed in volume context
   */
  HostOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX,
                    bool inheritable_ = false):
    Directive(name_, min_, max_),
    inheritable(inheritable_) {}
  void check(const ConfContext &cc) const override {
    if(cc.host == nullptr)
      throw SyntaxError("'" + name + "' command without 'host'");
    if(cc.volume != nullptr && !inheritable)
      throw SyntaxError("'" + name + "' inside 'volume'");
    Directive::check(cc);
  }

  /** @brief If @c true, also allowed in volume context */
  bool inheritable;
};

/** @brief Base class for directives that can only appear in a volume context */
struct VolumeOnlyDirective: public Directive {
  /** @brief Constructor
   *
   * @param name_ Name of directive
   * @param min_ Minimum number of arguments
   * @param max_ Maximum number of arguments
   */
  VolumeOnlyDirective(const char *name_, int min_=0, int max_=INT_MAX):
    Directive(name_, min_, max_) {}
  void check(const ConfContext &cc) const override {
    if(cc.volume == nullptr)
      throw SyntaxError("'" + name + "' command without 'volume'");
    Directive::check(cc);
  }
};

/** @brief Base class for color-setting directives */
struct ColorDirective: public Directive {
  /** @brief Constructor
   * @param name Name of directive
   */
  ColorDirective(const char *name): Directive(name, 1, 4) {}

  void check(const ConfContext &cc) const override {
    int args = cc.bits.size() - 1;
    Directive::check(cc);
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

  void set(ConfContext &cc) const override {
    int args = cc.bits.size() - 1;
    if(args == 4) {
      if(cc.bits[1] == "rgb")
        set_rgb(cc, 2);
      else if(cc.bits[1] == "hsv")
        set_hsv(cc, 2);
    } else
      set_packed(cc, 1, 0);
  }

  /** @brief Parse and set an RGB color representation
   * @param cc Configuration context
   * @param n Index for first element
   */
  void set_rgb(ConfContext &cc, size_t n) const {
    set(cc, Color(parseFloat(cc.bits[n], 0, 1),
                  parseFloat(cc.bits[n+1], 0, 1),
                  parseFloat(cc.bits[n+2], 0, 1)));
  }

  /** @brief Parse and set an HSV color representation
   * @param cc Configuration context
   * @param n Index for first element
   */
  void set_hsv(ConfContext &cc, size_t n) const {
    set(cc, Color::HSV(parseFloat(cc.bits[n]),
                       parseFloat(cc.bits[n+1], 0, 1),
                       parseFloat(cc.bits[n+2], 0, 1)));
  }

  /** @brief Parse and set a packed integer color representation
   * @param cc Configuration context
   * @param n Index for first element
   * @param radix Radix or 0 to pick as per strtol(3)
   */
  void set_packed(ConfContext &cc, size_t n, int radix = 0) const {
    set(cc, Color(parseInteger(cc.bits[n], 0, 0xFFFFFF, radix)));
  }

  /** @brief Set a color
   * @param cc Configuration context
   * @param c Color to set
   */
  virtual void set(ConfContext &cc, const Color &c) const = 0;

};

// Global directives ----------------------------------------------------------

/** @brief The @c store directive */
static const struct StoreDirective: public Directive {
  StoreDirective(): Directive("store", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->stores[cc.bits[1]] = new Store(cc.bits[1]);
  }
} store_directive;

/** @brief The @c store-pattern directive */
static const struct StorePatternDirective: public Directive {
  StorePatternDirective(): Directive("store-pattern", 1, 1) {}
  void set(ConfContext &cc) const override {
    std::vector<std::string> files;
    globFiles(files, cc.bits[1], GLOB_NOCHECK);
    for(auto &file: files)
      cc.conf->stores[file] = new Store(file);
  }
} store_pattern_directive;

/** @brief The @c stylesheet directive */
static const struct StyleSheetDirective: public Directive {
  StyleSheetDirective(): Directive("stylesheet", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->stylesheet = cc.bits[1];
  }
} stylesheet_directive;

/** @brief The @c colors directive */
static const struct ColorsDirective: public Directive {
  ColorsDirective(): Directive("colors", 2, 2) {}
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
static const struct DeviceDirective: public Directive {
  DeviceDirective(): Directive("device", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->devices[cc.bits[1]] = new Device(cc.bits[1]);
  }
} device_directive;

/** @brief The @c max-usage directive */
static const struct MaxUsageDirective: public Directive {
  MaxUsageDirective(): Directive("max-usage", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->maxUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_usage_directive;

/** @brief The @c max-file-usage directive */
static const struct MaxFileUsageDirective: public Directive {
  MaxFileUsageDirective(): Directive("max-file-usage", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->maxFileUsage = parseInteger(cc.bits[1], 0, 100);
  }
} max_file_usage_directive;

/** @brief The @c public directive */
static const struct PublicDirective: public Directive {
  PublicDirective(): Directive("public", 0, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->publicStores = get_boolean(cc);
  }
} public_directive;

/** @brief The @c logs directive */
static const struct LogsDirective: public Directive {
  LogsDirective(): Directive("logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->logs = cc.bits[1];
  }
} logs_directive;

/** @brief The @c lock directive */
static const struct LockDirective: public Directive {
  LockDirective(): Directive("lock", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->lock = cc.bits[1];
  }
} lock_directive;

/** @brief The @c sendmail directive */
static const struct SendmailDirective: public Directive {
  SendmailDirective(): Directive("sendmail", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->sendmail = cc.bits[1];
  }
} sendmail_directive;

/** @brief The @c pre-access-hook directive */
static const struct PreAccessHookDirective: public Directive {
  PreAccessHookDirective(): Directive("pre-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->preAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_access_hook_directive;

/** @brief The @c post-access-hook directive */
static const struct PostAccessHookDirective: public Directive {
  PostAccessHookDirective(): Directive("post-access-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.conf->postAccess.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_access_hook_directive;

/** @brief The @c keep-prune-logs directive */
static const struct KeepPruneLogsDirective: public Directive {
  KeepPruneLogsDirective(): Directive("keep-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->keepPruneLogs = parseInteger(cc.bits[1], 1);
  }
} keep_prune_logs_directive;

/** @brief The @c report-prune-logs directive */
static const struct ReportPruneLogsDirective: public Directive {
  ReportPruneLogsDirective(): Directive("report-prune-logs", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.conf->reportPruneLogs = parseInteger(cc.bits[1], 1);
  }
} report_prune_logs_directive;

/** @brief The @c include directive */
static const struct IncludeDirective: public Directive {
  IncludeDirective(): Directive("include", 1, 1) {}
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
static const struct DeviceColorStrategyDirective: public Directive {
  DeviceColorStrategyDirective(): Directive("device-color-strategy",
                                            1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    ColorStrategy *nc = ColorStrategy::create(cc.bits[1], cc.bits, 2);
    delete cc.conf->deviceColorStrategy;
    cc.conf->deviceColorStrategy = nc;
  }
} device_color_strategy_directive;

/** @brief The horizontal-padding directive */
static const struct HorizontalPaddingDirective: public Directive {
  HorizontalPaddingDirective(): Directive("horizontal-padding") {}
  void set(ConfContext &cc) const override {
    cc.conf->horizontalPadding = parseFloat(cc.bits[1],
                                            0,
                                            std::numeric_limits<double>::max());
  }
} horizontal_padding_directive;

/** @brief The vertical-padding directive */
static const struct VerticalPaddingDirective: public Directive {
  VerticalPaddingDirective(): Directive("vertical-padding") {}
  void set(ConfContext &cc) const override {
    cc.conf->verticalPadding = parseFloat(cc.bits[1],
                                          0,
                                          std::numeric_limits<double>::max());
  }
} vertical_padding_directive;

/** @brief The backup-indicator-width directive */
static const struct BackupIndicatorWidthDirective: public Directive {
  BackupIndicatorWidthDirective(): Directive("backup-indicator-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorWidth
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_width_directive;

/** @brief The backup-indicator-height directive */
static const struct BackupIndicatorHeightDirective: public Directive {
  BackupIndicatorHeightDirective(): Directive("backup-indicator-height") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorHeight
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_height_directive;

/** @brief The backup-indicator-key-width directive */
static const struct BackupIndicatorKeyWidthDirective: public Directive {
  BackupIndicatorKeyWidthDirective(): Directive("backup-indicator-key-width") {}
  void set(ConfContext &cc) const override {
    cc.conf->backupIndicatorKeyWidth
      = parseFloat(cc.bits[1],
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max());
  }
} backup_indicator_key_width_directive;

// Inheritable directives -----------------------------------------------------

/** @brief The @c max-age directive */
static const struct MaxAgeDirective: public Directive {
  MaxAgeDirective(): Directive("max-age", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->maxAge = parseInteger(cc.bits[1], 1);
  }
} max_age_directive;

/** @brief The @c min-backups directive */
static const struct MinBackupsDirective: public Directive {
  MinBackupsDirective(): Directive("min-backups", 1, 1) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'min-backups' directive is deprecated, use 'prune-parameter min-backups' instead",
            cc.path.c_str(), cc.line);
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["min-backups"] = cc.bits[1];
  }
} min_backups_directive;

/** @brief The @c prune-age directive */
static const struct PruneAgeDirective: public Directive {
  PruneAgeDirective(): Directive("prune-age", 1, 1) {}
  void set(ConfContext &cc) const override {
    warning("%s:%d: the 'prune-age' directive is deprecated, use 'prune-parameter prune-age' instead",
            cc.path.c_str(), cc.line);
    parseInteger(cc.bits[1], 1);
    cc.context->pruneParameters["prune-age"] = cc.bits[1];
  }
} prune_age_directive;

/** @brief The @c prune-policy directive */
static const struct PrunePolicyDirective: public Directive {
  PrunePolicyDirective(): Directive("prune-policy", 1, 1) {}
  void set(ConfContext &cc) const override {
    if(cc.bits[1].size() > 0 && cc.bits[1].at(0) == '/') {
      cc.context->prunePolicy = "exec";
      cc.context->pruneParameters["path"] = cc.bits[1];
    } else
      cc.context->prunePolicy = cc.bits[1];
  }
} prune_policy_directive;

/** @brief The @c prune-parameter directive */
static const struct PruneParameterDirective: public Directive {
  PruneParameterDirective(): Directive("prune-parameter", 2, 2) {}
  void check(const ConfContext &cc) const override {
    Directive::check(cc);
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
static const struct PreBackupHookDirective: public Directive {
  PreBackupHookDirective(): Directive("pre-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->preBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} pre_backup_hook_directive;

/** @brief The @c post-backup-hook directive */
static const struct PostBackupHookDirective: public Directive {
  PostBackupHookDirective(): Directive("post-backup-hook", 1, INT_MAX) {}
  void set(ConfContext &cc) const override {
    cc.context->postBackup.assign(cc.bits.begin() + 1, cc.bits.end());
  }
} post_backup_hook_directive;

/** @brief The @c rsync-timeout directive */
static const struct RsyncTimeoutDirective: public Directive {
  RsyncTimeoutDirective(): Directive("rsync-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->rsyncTimeout = parseInteger(cc.bits[1], 1);
  }
} rsync_timeout_directive;

/** @brief The @c hook-timeout directive */
static const struct HookTimeoutDirective: public Directive {
  HookTimeoutDirective(): Directive("hook-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->hookTimeout = parseInteger(cc.bits[1], 1);
  }
} hook_timeout_directive;

/** @brief The @c ssh-timeout directive */
static const struct SshTimeoutDirective: public Directive {
  SshTimeoutDirective(): Directive("ssh-timeout", 1, 1) {}
  void set(ConfContext &cc) const override {
    cc.context->sshTimeout = parseInteger(cc.bits[1], 1);
  }
} ssh_timeout_directive;

// Host directives ------------------------------------------------------------

/** @brief The @c host directive */
static const struct HostDirective: public Directive {
  HostDirective(): Directive("host", 1, 1) {}
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

Conf::Conf() {
  std::vector<std::string> args;
  args.push_back("120");
  args.push_back("0.75");
  deviceColorStrategy = ColorStrategy::create(DEFAULT_COLOR_STRATEGY, args);
}

void Conf::write(std::ostream &os, int step, bool verbose) const {
  describe_type *d = verbose ? describe : nodescribe;

  d(os, "# ---- Inheritable directives ----", step);
  d(os, "", step);

  ConfBase::write(os, step, verbose);

  d(os, "# ---- Non-inheritable directives ----", step);
  d(os, "", step);

  d(os, "# Whether stores are public or private (default)", step);
  d(os, "#  public true|false", step);
  os << indent(step) << "public " << (publicStores ? "true" : "false") << '\n';
  d(os, "", step);

  d(os, "# Path to log directory", step);
  d(os, "#  logs PATH", step);
  os << indent(step) << "logs " << quote(logs) << '\n';
  d(os, "", step);

  d(os, "# Path to lock file", step);
  d(os, "#  lock PATH", step);
  if(lock.size())
    os << indent(step) << "lock " << quote(lock) << '\n';
  d(os, "", step);

  d(os, "# Command to run before accessing backup devices", step);
  d(os, "#  pre-access-hook COMMAND ...", step);
  if(preAccess.size())
    os << indent(step) << "pre-access-hook " << quote(preAccess) << '\n';
  d(os, "", step);

  d(os, "# Command to run after accessing backup devices", step);
  d(os, "#  pre-access-hook COMMAND ...", step);
  if(postAccess.size())
    os << indent(step) << "post-access-hook " << quote(postAccess) << '\n';
  d(os, "", step);

  d(os, "# Names of backup devices", step);
  d(os, "#  device NAME", step);
  for(auto &d: devices)
    os << "device " << quote(d.first) << '\n';
  d(os, "", step);

  d(os, "# ---- Reporting ----", step);
  d(os, "", step);

  d(os, "# 'Good' and 'bad' colors for HTML report", step);
  d(os, "#  color-good 0xRRGGBB", step);
  d(os, "#  color-bad 0xRRGGBB", step);
  os << indent(step) << "color-good 0x" << colorGood << '\n'
     << indent(step) << "color-bad 0x" << colorBad << '\n';
  d(os, "", step);

  d(os, "# How many days worth of pruning logs to report", step);
  d(os, "#  report-prune-logs DAYS", step);
  os << indent(step) << "report-prune-logs " << reportPruneLogs << '\n';
  d(os, "", step);

  d(os, "# Path to mail transport agent", step);
  d(os, "#  sendmail PATH", step);
  os << indent(step) << "sendmail " << quote(sendmail) << '\n';
  d(os, "", step);

  d(os, "# Stylesheet for HTML report", step);
  d(os, "#  stylesheet PATH", step);
  if(stylesheet.size())
    os << indent(step) << "stylesheet " << quote(stylesheet) << '\n';
  d(os, "", step);

  d(os, "# ---- Graphs ----", step);
  d(os, "", step);

  d(os, "# Graph background color", step);
  d(os, "#  color-graph-background 0xRRGGBB", step);
  os << indent(step) << "color-graph-background 0x" << colorGraphBackground << '\n';
  d(os, "", step);

  d(os, "# Graph foreground color", step);
  d(os, "#  color-graph-foreground 0xRRGGBB", step);
  os << indent(step) << "color-graph-foreground 0x" << colorGraphForeground << '\n';
  d(os, "", step);

  d(os, "# Graph month guide color", step);
  d(os, "#  color-month-guide 0xRRGGBB", step);
  os << indent(step) << "color-month-guide 0x" << colorMonthGuide << '\n';
  d(os, "", step);

  d(os, "# Graph host guide color", step);
  d(os, "#  color-host-guide 0xRRGGBB", step);
  os << indent(step) << "color-host-guide 0x" << colorHostGuide << '\n';
  d(os, "", step);

  d(os, "# Graph volume guide color", step);
  d(os, "#  color-volume-guide 0xRRGGBB", step);
  os << indent(step) << "color-volume-guide 0x" << colorVolumeGuide << '\n';
  d(os, "", step);

  d(os, "# Strategy for picking device colors", step);
  d(os, "#  device-color-strategy equidistant-value HUE", step);
  d(os, "#  device-color-strategy equidistant-value HUE SATURATION", step);
  d(os, "#  device-color-strategy equidistant-value HUE SATURATION MINVALUE MAXVALUE", step);
  d(os, "#  device-color-strategy equidistant-hue HUE", step);
  d(os, "#  device-color-strategy equidistant-hue HUE SATURATION VALUE", step);
  os << indent(step) << "device-color-strategy "
     << deviceColorStrategy->description() << '\n';
  d(os, "", step);

  d(os, "# Horizontal padding", step);
  d(os, "#  horizontal-padding PIXELS", step);
  os << indent(step) << "horizontal-padding " << horizontalPadding << '\n';
  d(os, "", step);

  d(os, "# Vertical padding", step);
  d(os, "#  vertical-padding PIXELS", step);
  os << indent(step) << "vertical-padding " << verticalPadding << '\n';
  d(os, "", step);

  d(os, "# Width of a backup indicator", step);
  d(os, "#  backup-indicator-width PIXELS", step);
  os << indent(step) << "backup-indicator-width " << backupIndicatorWidth << '\n';
  d(os, "", step);

  d(os, "# Minimum height of a backup indicator ", step);
  d(os, "#  backup-indicator-height PIXELS", step);
  os << indent(step) << "backup-indicator-height " << backupIndicatorHeight << '\n';
  d(os, "", step);

  d(os, "# Width of a backup indicator in the device key", step);
  d(os, "#  backup-indicator-key-width PIXELS", step);
  os << indent(step) << "backup-indicator-key-width " << backupIndicatorKeyWidth << '\n';
  d(os, "", step);


  d(os, "# ---- Hosts to back up ----", step);

  for(auto &h: hosts) {
    os << '\n';
    h.second->write(os, step, verbose);
  }
}

// Read the master configuration file plus anything it includes.
void Conf::read() {
  readOneFile(configPath);
}

// Read one configuration file.  Throws IOError if some file cannot be
// read or ConfigError if the contents are bad.
void Conf::readOneFile(const std::string &path) {
  ConfContext cc(this);

  IO input;
  D("Conf::readOneFile %s", path.c_str());
  input.open(path, "r");

  std::string line;
  int lineno = 0;
  while(input.readline(line)) {
    ++lineno;                           // keep track of where we are
    cc.path = path;
    cc.line = lineno;
    try {
      split(cc.bits, line);
      if(!cc.bits.size())                  // skip blank lines
        continue;
      // Consider all the possible commands
      auto it = (*directives).find(cc.bits[0]);
      if(it != (*directives).end()) {
        const Directive *d = it->second;
        d->check(cc);
        d->set(cc);
      } else {
        throw SyntaxError("unknown command '" + cc.bits[0] + "'");
      }
    } catch(SyntaxError &e) {
      // Wrap up in a ConfigError, which carries the path/line information.
      std::stringstream s;
      s << path << ":" << lineno << ": " << e.what();
      throw ConfigError(s.str());
    }
  }
}

// Implementation of the 'include' command.  If PATH is a directory then
// includes all the regular files it contains (excluding dotfiles and backups
// but including symbolic links to regular files of any name), otherwise just
// tries to read it.
void Conf::includeFile(const std::string &path) {
  D("Conf::includeFile %s", path.c_str());
  if(boost::filesystem::is_directory(path)) {
    std::vector<std::string> files;
    Directory::getFiles(path, files);
    for(auto &name: files) {
      if(!name.size()
         || name.at(0) == '.'
         || name.at(0) == '#'
         || name.find('~') != std::string::npos)
        continue;
      std::string fullname = path + PATH_SEP + name;
      if(boost::filesystem::is_regular_file(fullname))
        readOneFile(fullname);
    }
  } else
    readOneFile(path);
}

void Conf::validate() const {
  for(auto &h: hosts)
    for(auto &v: h.second->volumes)
      validatePrunePolicy(v.second);
}

// (De-)select all hosts
void Conf::selectAll(bool sense) {
  for(auto &h: hosts)
    h.second->select(sense);
}

// (De-)select one host (or all if hostName="*")
void Conf::selectHost(const std::string &hostName, bool sense) {
  if(hostName == "*") {
    selectAll(sense);
  } else {
    auto hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw CommandError("no such host as '" + hostName + "'");
    hosts_iterator->second->select(sense);
  }
}

// (De-)select one volume (or all if volumeName="*")
void Conf::selectVolume(const std::string &hostName,
                        const std::string &volumeName,
                        bool sense) {
  if(volumeName == "*") {
    selectHost(hostName, sense);
  } else {
    auto hosts_iterator = hosts.find(hostName);
    if(hosts_iterator == hosts.end())
      throw CommandError("no such host as '" + hostName + "'");
    Host *host = hosts_iterator->second;
    auto volumes_iterator = host->volumes.find(volumeName);
    if(volumes_iterator == host->volumes.end())
      throw CommandError("no such volume as '" + hostName
                         + ":" + volumeName + "'");
    volumes_iterator->second->select(sense);
  }
}

void Conf::addHost(Host *h) {
  hosts[h->name] = h;
}

// Find a host by name
Host *Conf::findHost(const std::string &hostName) const {
  auto it = hosts.find(hostName);
  return it != hosts.end() ? it->second : nullptr;
}

// Find a volume by name
Volume *Conf::findVolume(const std::string &hostName,
                         const std::string &volumeName) const {
  Host *host = findHost(hostName);
  return host ? host->findVolume(volumeName) : nullptr;
}

// Find a device by name
Device *Conf::findDevice(const std::string &deviceName) const {
  auto it = devices.find(deviceName);
  return it != devices.end() ? it->second : nullptr;
}

// Read in logfiles
void Conf::readState() {
  if(logsRead)
    return;
  std::string hostName, volumeName;
  std::vector<std::string> files;
  const bool progress = command.verbose && isatty(2);
  std::vector<std::string> upgraded;

  std::string log;
  // Read database contents
  // Better would be to read only the rows required, on demand.
  {
    Database::Statement stmt(getdb(),
                             "SELECT host,volume,device,id,time,pruned,rc,status,log"
                             " FROM backup",
                             SQL_END);
    while(stmt.next()) {
      Backup backup;
      hostName = stmt.get_string(0);
      volumeName = stmt.get_string(1);
      backup.deviceName = stmt.get_string(2);
      backup.id = stmt.get_string(3);
      backup.time = stmt.get_int64(4);
      backup.date = Date(backup.time);
      backup.pruned = stmt.get_int64(5);
      backup.rc = stmt.get_int(6);
      backup.setStatus(stmt.get_int(7));
      backup.contents = stmt.get_blob(8);
      addBackup(backup, hostName, volumeName);
    }
  }

  // Upgrade old-format logfiles
  if(boost::filesystem::exists(logs)) {
    Directory::getFiles(logs, files);
    std::regex logfileRegexp("^([0-9]+-[0-9]+-[0-9]+)-([^-]+)-([^-]+)-([^-]+)\\.log$");
    for(size_t n = 0; n < files.size(); ++n) {
      if(progress)
        progressBar(IO::err, "Upgrading old logs", n, files.size());
      // Parse the filename
      std::smatch mr;
      if(!std::regex_match(files[n], mr, logfileRegexp))
        continue;
      Backup backup;
      backup.date = Date(mr[1]);
      backup.id = mr[1];
      backup.time = backup.date.toTime();
      backup.deviceName = mr[2];
      hostName = mr[3];
      volumeName = mr[4];

      // Read the log
      IO input;
      std::vector<std::string> contents;
      input.open(logs + "/" + files[n], "r");
      input.readlines(contents);
      // Skip empty files
      if(contents.size() == 0) {
        if(progress)
          progressBar(IO::err, nullptr, 0, 0);
        warning("empty file: %s", files[n].c_str());
        continue;
      }
      // Find the status code
      const std::string &last = contents[contents.size() - 1];
      backup.rc = -1;
      if(last.compare(0, 3, "OK:") == 0)
        backup.rc = 0;
      else {
        std::string::size_type pos = last.rfind("error=");
        if(pos < std::string::npos)
          sscanf(last.c_str() + pos + 6, "%i", &backup.rc);
      }
      if(last.rfind("pruning") != std::string::npos)
        backup.setStatus(PRUNING);
      else if(backup.rc == 0)
        backup.setStatus(COMPLETE);
      else
        backup.setStatus(FAILED);
      for(std::string &c: contents) {
        backup.contents += c;
        backup.contents += "\n";
      }

      addBackup(backup, hostName, volumeName, true);

      if(command.act) {
        // addBackup might fail to set volume
        if(backup.volume != nullptr) {
          if(upgraded.size() == 0)
            getdb().begin();
          try {
            backup.insert(getdb());
          } catch(DatabaseError &e) {
            if(e.status != SQLITE_CONSTRAINT)
              throw;
          }
          upgraded.push_back(files[n]);
        } else {
          if(progress)
            progressBar(IO::err, nullptr, 0, 0);
          warning("cannot upgrade %s", files[n].c_str());
        }
      }
    }
    logsRead = true;
    if(command.act && upgraded.size()) {
      getdb().commit();
      bool upgradeFailure = false;
      for(std::string &u: upgraded) {
        const std::string path = logs + "/" + u;
        if(unlink(path.c_str())) {
          error("removing %s: %s", path.c_str(), strerror(errno));
          upgradeFailure = true;
        }
      }
      if(upgradeFailure)
        throw SystemError("could not remove old logfiles");
    }
  }
  if(progress)
    progressBar(IO::err, nullptr, 0, 0);
}

void Conf::addBackup(Backup &backup,
                     const std::string &hostName,
                     const std::string &volumeName,
                     bool forceWarn) {
  const bool progress = command.verbose && isatty(2);

  /* Don't keep pruned backups around */
  if(backup.getStatus() == PRUNED)
    return;

  if(!contains(devices, backup.deviceName)) {
    if(!contains(unknownDevices, backup.deviceName)) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, nullptr, 0, 0);
        warning("unknown device %s", backup.deviceName.c_str());
      }
      unknownDevices.insert(backup.deviceName);
      ++config.unknownObjects;
    }
    return;
  }
  // Find the volume for this status record.  If it cannot be found, we warn
  // about it once.
  Host *host = findHost(hostName);
  if(!host) {
    if(!contains(unknownHosts, hostName)) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, nullptr, 0, 0);
        warning("unknown host %s", hostName.c_str());
      }
      unknownHosts.insert(hostName);
      ++config.unknownObjects;
    }
    return;
  }
  Volume *volume = host->findVolume(volumeName);
  if(!volume) {
    if(!contains(host->unknownVolumes, volumeName)) {
      if(command.warnUnknown || forceWarn) {
        if(progress)
          progressBar(IO::err, nullptr, 0, 0);
        warning("unknown volume %s:%s",
                hostName.c_str(), volumeName.c_str());
      }
      host->unknownVolumes.insert(volumeName);
      ++config.unknownObjects;
    }
    return;
  }
  backup.volume = volume;
  // Attach the status record to the volume
  volume->addBackup(new Backup(backup));
}

// Create the mapping between stores and devices.
void Conf::identifyDevices(int states) {
  if((devicesIdentified & states) == states)
    return;
  int found = 0;
  std::vector<UnavailableStore> storeExceptions;
  for(auto &s: stores) {
    Store *store = s.second;
    if(!(store->state & states))
      continue;
    try {
      store->identify();
      ++found;
    } catch(UnavailableStore &unavailableStoreException) {
      if(command.warnStore)
        warning("%s", unavailableStoreException.what());
      storeExceptions.push_back(unavailableStoreException);
    } catch(FatalStoreError &fatalStoreException) {
      if(states == Store::Enabled)
        throw;
      else if(command.warnStore)
        warning("%s", fatalStoreException.what());
    } catch(BadStore &badStoreException) {
      if(states == Store::Enabled)
        error("%s", badStoreException.what());
    }
  }
  if(!found && states == Store::Enabled) {
    error("no backup devices found");
    if(!command.warnStore)
      for(size_t n = 0; n < storeExceptions.size(); ++n)
        IO::err.writef("  %s\n", storeExceptions[n].what());
  }
  devicesIdentified |= states;
}

Database &Conf::getdb() {
  if(!db) {
    if(database.size() == 0)
      database = logs + "/backups.db";
    if(command.act) {
      db = new Database(database);
      if(!db->hasTable("backup"))
        createTables();
    } else {
      try {
        db = new Database(database, false);
      } catch(DatabaseError &) {
        db = new Database(":memory:");
        createTables();
      }
    }
  }
  return *db;
}

void Conf::createTables() {
  db->begin();
  db->execute("CREATE TABLE backup (\n"
              "  host TEXT,\n"
              "  volume TEXT,\n"
              "  device TEXT,\n"
              "  id TEXT,\n"
              "  time INTEGER,\n"
              "  pruned INTEGER,\n"
              "  rc INTEGER,\n"
              "  status INTEGER,\n"
              "  log BLOB,\n"
              "  PRIMARY KEY (host,volume,device,id)\n"
              ")");
  db->commit();
}

ConfBase *Conf::getParent() const {
  return nullptr;
}

std::string Conf::what() const {
  return "system";
}

Conf config;
