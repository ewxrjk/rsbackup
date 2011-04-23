//-*-C++-*-
#ifndef EMAIL_H
#define EMAIL_H

#include <string>
#include <vector>

class Email {
public:
  Email();
  
  void addTo(const std::string &address) { to.push_back(address); }
  void setFrom(const std::string &address) { from = address; }
  void setSubject(const std::string &text) { subject = text; }
  void setType(const std::string &type_) { type = type_; }
  void setContent(const std::string &msg) { content = msg; }

  void send() const;
private:
  std::string from, subject;
  std::vector<std::string> to;
  std::string type;
  std::string content;
};

#endif /* EMAIL_H */
