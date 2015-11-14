//-*-C++-*-
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
#ifndef EMAIL_H
#define EMAIL_H
/** @file Email.h
 * @brief Constructing and sending email
 */

#include <string>
#include <vector>

/** @brief An email message */
class Email {
public:
  /** @brief Add a recipient
   * @param address Destination address
   */
  void addTo(const std::string &address) { to.push_back(address); }

  /** @brief Set sender
   * @param address Sender address
   */
  void setFrom(const std::string &address) { from = address; }

  /** @brief Set subject
   * @param text Subject
   */
  void setSubject(const std::string &text) { subject = text; }

  /** @brief Set content type
   * @param type_ Content type
   *
   * The default is @c text/plain (with no indication of charset).
   */
  void setType(const std::string &type_) { type = type_; }

  /** @brief Set content
   * @param msg Content
   */
  void setContent(const std::string &msg) { content = msg; }

  /** @brief Send message */
  void send() const;

private:
  /** @brief Sender address */
  std::string from;

  /** @brief Subject */
  std::string subject;

  /** @brief Recipients */
  std::vector<std::string> to;

  /** @brief MIME Content type */
  std::string type = "text/plain";

  /** @brief Contents of email */
  std::string content;
};

#endif /* EMAIL_H */
