// Copyright © Richard Kettlewell.
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
#include "Command.h"
#include "Conf.h"
#include "Store.h"
#include "Errors.h"
#include "Document.h"
#include "Email.h"
#include "IO.h"
#include "FileLock.h"
#include "Subprocess.h"
#include "DeviceAccess.h"
#include "Utils.h"
#include "Report.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <regex>

std::mutex globalLock;

static void commandLineStores(const std::vector<std::string> &stores,
                              bool mounted);

int main(int argc, char **argv) {
  // The global lock is normally held by the main thread
  globalLock.lock();
  try {
    if(setlocale(LC_CTYPE, "") == nullptr)
      throw std::runtime_error(std::string("setlocale: ") + strerror(errno));

    // Parse command line
    globalCommand.parse(argc, argv);

    // Read configuration
    globalConfig.read();

    // Validate configuration
    globalConfig.validate();

    // Dump configuration
    if(globalCommand.dumpConfig) {
      globalConfig.write(std::cout, 0, !!(globalWarningMask & WARNING_VERBOSE));
      exit(0);
    }

    // Override stores
    if(globalCommand.stores.size() != 0
       || globalCommand.unmountedStores.size() != 0) {
      for(auto &s: globalConfig.stores)
        s.second->state = Store::Disabled;
      commandLineStores(globalCommand.stores, true);
      commandLineStores(globalCommand.unmountedStores, false);
    }

    // Take the lock, if one is defined.
    FileLock lockFile(globalConfig.lock);
    if((globalCommand.backup || globalCommand.prune
        || globalCommand.pruneIncomplete || globalCommand.retireDevice
        || globalCommand.retire)
       && globalConfig.lock.size()) {
      D("attempting to acquire lockfile %s", globalConfig.lock.c_str());
      if(!lockFile.acquire(globalCommand.wait)) {
        // Failing to acquire the lock is not really an error if --wait was not
        // requested.
        warning(WARNING_VERBOSE, "cannot acquire lockfile %s",
                globalConfig.lock.c_str());
        exit(0);
      }
    }

    // Select volumes
    if(globalCommand.backup || globalCommand.prune
       || globalCommand.pruneIncomplete)
      globalCommand.selections.select(globalConfig);

    // Execute commands
    if(globalCommand.backup)
      makeBackups();
    if(globalCommand.retire)
      retireVolumes(!globalCommand.forgetOnly);
    if(globalCommand.retireDevice)
      retireDevices();
    if(globalCommand.prune || globalCommand.pruneIncomplete)
      pruneBackups();
    if(globalCommand.prune)
      prunePruneLogs();
    if(globalCommand.checkUnexpected)
      checkUnexpected();
    if(globalCommand.latest)
      findLatest();

    // Run post-device hook
    postDeviceAccess();

    // Generate report
    if(globalCommand.html || globalCommand.text || globalCommand.email) {
      globalConfig.readState();

      Document d;
      if(globalConfig.stylesheet.size()) {
        IO ssf;
        ssf.open(globalConfig.stylesheet, "r");
        ssf.readall(d.htmlStyleSheet);
      } else
        d.htmlStyleSheet = stylesheet;
      // Include user colors in the stylesheet
      std::stringstream ss;
      ss << "td.bad { background-color: #" << globalConfig.colorBad << " }\n";
      ss << "td.good { background-color: #" << globalConfig.colorGood << " }\n";
      ss << "span.bad { color: #" << globalConfig.colorBad << " }\n";
      d.htmlStyleSheet += ss.str();
      Report report(d);
      report.generate();
      if(globalCommand.html) {
        std::stringstream htmlStream;
        RenderDocumentContext htmlRenderDocumentContext;
        d.renderHtml(htmlStream, nullptr);
        if(*globalCommand.html == "-") {
          IO::out.write(htmlStream.str());
        } else {
          IO f;
          f.open(*globalCommand.html, "w");
          f.write(htmlStream.str());
          f.close();
        }
      }
      if(globalCommand.text) {
        IO *f;
        if(*globalCommand.text == "-") {
          f = &IO::out;
        } else {
          f = new IO();
          f->open(*globalCommand.text, "w");
        }
        std::stringstream textStream;
        RenderDocumentContext textRenderDocumentContext;
        int w = f->width();
        if(w)
          textRenderDocumentContext.width = w;
        d.renderText(textStream, &textRenderDocumentContext);
        f->write(textStream.str());
        if(f != &IO::out)
          f->close();
      }
      if(globalCommand.email) {
        std::stringstream htmlStream, textStream;
        RenderDocumentContext htmlRenderDocumentContext;
        RenderDocumentContext textRenderDocumentContext;
        d.renderHtml(htmlStream, &htmlRenderDocumentContext);
        d.renderText(textStream, &textRenderDocumentContext);
        Email e;
        e.addTo(*globalCommand.email);
        std::stringstream subject;
        subject << d.title;
        if(report.backups_missing)
          subject << " missing:" << report.backups_missing;
        if(report.backups_partial)
          subject << " partial:" << report.backups_partial;
        if(report.backups_out_of_date)
          subject << " stale:" << report.backups_out_of_date;
        if(report.backups_failed)
          subject << " failed:" << report.backups_failed;
        if(report.devices_unknown || report.hosts_unknown
           || report.volumes_unknown)
          subject << " unknown:"
                  << (report.devices_unknown + report.hosts_unknown
                      + report.volumes_unknown);
        e.setSubject(subject.str());
        e.setType("multipart/related; boundary=" MIME_BOUNDARY MIME1);
        std::stringstream body;
        body << "--" MIME_BOUNDARY MIME1 "\n";
        body << "Content-Type: multipart/alternative; boundary=" MIME_BOUNDARY
                MIME2 "\n";
        body << "\n";
        body << "--" MIME_BOUNDARY MIME2 "\n";
        body << "Content-Type: text/plain\n";
        body << "\n";
        body << textStream.str();
        body << "\n";
        body << "--" MIME_BOUNDARY MIME2 "\n";
        body << "Content-Type: text/html\n";
        body << "\n";
        body << htmlStream.str();
        body << "\n";
        body << "--" MIME_BOUNDARY MIME2 "--\n";
        body << "\n";
        for(auto image: htmlRenderDocumentContext.images) {
          body << "--" MIME_BOUNDARY MIME1 "\n";
          body << "Content-ID: <" << image->ident() << ">\n";
          body << "Content-Type: " << image->type << "\n";
          body << "Content-Transfer-Encoding: base64\n";
          body << "\n";
          std::stringstream ss;
          write_base64(ss, image->content);
          body << ss.str();
          body << "\n";
        }
        body << "--" MIME_BOUNDARY MIME1 "--\n";
        e.setContent(body.str());
        e.send();
      }
    }
    if(globalErrors)
      warning(WARNING_VERBOSE, "%d errors detected", globalErrors);
    IO::out.close();
  } catch(Error &e) {
    error("%s", e.what());
    if(globalDebug)
      e.trace(stderr);
  } catch(std::regex_error &e) {
    switch(e.code()) {
    case std::regex_constants::error_collate:
      error("std::regex_constants::error_collate");
      break;
    case std::regex_constants::error_ctype:
      error("std::regex_constants::error_ctype");
      break;
    case std::regex_constants::error_escape:
      error("std::regex_constants::error_escape");
      break;
    case std::regex_constants::error_backref:
      error("std::regex_constants::error_backref");
      break;
    case std::regex_constants::error_brack:
      error("std::regex_constants::error_brack");
      break;
    case std::regex_constants::error_paren:
      error("std::regex_constants::error_paren");
      break;
    case std::regex_constants::error_brace:
      error("std::regex_constants::error_brace");
      break;
    case std::regex_constants::error_badbrace:
      error("std::regex_constants::error_badbrace");
      break;
    case std::regex_constants::error_range:
      error("std::regex_constants::error_range");
      break;
    case std::regex_constants::error_space:
      error("std::regex_constants::error_space");
      break;
    case std::regex_constants::error_badrepeat:
      error("std::regex_constants::error_badrepeat");
      break;
    case std::regex_constants::error_complexity:
      error("std::regex_constants::error_complexity");
      break;
    case std::regex_constants::error_stack:
      error("std::regex_constants::error_stack");
      break;
    default: error("regex error code %d", e.code()); break;
    }
  } catch(std::runtime_error &e) {
    error("%s", e.what());
  }
  exit(!!globalErrors);
}

static void commandLineStores(const std::vector<std::string> &stores,
                              bool mounted) {
  for(auto &s: stores) {
    auto it = globalConfig.stores.find(s);
    if(it == globalConfig.stores.end())
      globalConfig.stores[s] = new Store(s, mounted);
    else
      it->second->state = Store::Enabled;
  }
}
