#ifndef __ARGPARSE_H_
#define __ARGPARSE_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace heat_transfer {

class Argument {
 public:
  Argument(const std::string &opt, const std::string &desc, bool req)
      : option_(opt), description_(desc), required_(req) {}

  const std::string &option() const {
    return option_;
  }

  bool required() const {
    return required_;
  }

  friend std::ostream &operator<< (std::ostream &stream, const Argument &arg) {
    stream << "\t" << arg.option_ << "\t\t" << arg.description_;
    if (arg.required_) stream << " [required]";
    stream << std::endl;
    return stream;
  }

 protected:
  std::string option_;
  std::string description_;
  bool required_;
};

class ParsedArgument : public Argument {
 public:
  ParsedArgument(const Argument &arg, const std::string &value)
      : Argument(arg), value_(value) {}

  template<typename T>
  T value() const {
    T value;
    std::istringstream ss(value_);
    ss >> value;
    return value;
  }

 private:
  std::string value_;
};

class ArgumentParser {
 public:
  ArgumentParser(std::string prog_name, std::string prog_desc)
      : prog_name_(prog_name), prog_desc_(prog_desc) {}

  int AddArgument(std::string opt, std::string desc, bool req) {
    args_.push_back(Argument(opt, desc, req));
    return 0;
  }

  int Parse(int argc, char *argv[]) {
    std::vector<std::string> tokens;
    for (int i = 1; i != argc; ++i)
      tokens.push_back(argv[i]);

    std::vector<std::string>::const_iterator it;
    for (unsigned int i = 0; i != args_.size(); ++i) {
      it = std::find(tokens.begin(), tokens.end(), args_[i].option());
      if ((it == tokens.end() || ++it == tokens.end()) && args_[i].required()) {
        PrintHelp();
        std::cerr << "Error: Missing required option: " << args_[i].option()
                  << std::endl;
        return 1;
      } else {
        parsed_args_.insert(
            std::make_pair(args_[i].option(), ParsedArgument(args_[i], *it)));
      }
    }
    return 0;
  }

  template<typename T>
  T GetValue(std::string option) const {
    std::map<std::string, ParsedArgument>::const_iterator it;
    it = parsed_args_.find(option);
    return it->second.value<T>();
  }

  void PrintHelp() const {
    std::cerr << prog_name_ << ": " << prog_desc_ << std::endl << std::endl;
    std::cerr << "Usage:\n\t" << prog_name_ << " [options]\n" << std::endl;
    std::cerr << "Valid options: " << std::endl;
    for (unsigned int i = 0; i != args_.size(); ++i) {
      std::cerr << args_[i];
    }
    std::cerr << std::endl;
  }

 private:
  std::string prog_name_;
  std::string prog_desc_;
  std::vector<Argument> args_;
  std::map<std::string, ParsedArgument> parsed_args_;
};

}  // namespace heat_transfer

#endif  // __ARGPARSE_H_
