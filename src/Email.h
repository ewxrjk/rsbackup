//-*-C++-*-
#ifndef EMAIL_H
#define EMAIL_H

#include <string>
#include <vector>

// Simple representation of an email
class Email {
public:
  Email();

  // Add recipients.  Each must be in an acceptable format for a To: field.
  void addTo(const std::string &address) { to.push_back(address); }
  // Set sender.
  void setFrom(const std::string &address) { from = address; }
  // Set subject.
  void setSubject(const std::string &text) { subject = text; }
  // Set content type.  The default is text/plain (with no indication of
  // charset).
  void setType(const std::string &type_) { type = type_; }
  // Set content.
  void setContent(const std::string &msg) { content = msg; }

  // Send the email.
  void send() const;
private:
  std::string from, subject;
  std::vector<std::string> to;
  std::string type;
  std::string content;
};

#endif /* EMAIL_H */
