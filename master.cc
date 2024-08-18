#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <range/v3/action/split.hpp>

class Term final {
public:
    Term() : fdm_{open("/dev/ptmx", O_RDWR)}
    {
        if (fdm_ == -1) throw std::runtime_error("failed to open /dev/ptmx");

        if (grantpt(fdm_) == -1) throw std::runtime_error("failed to set slave permissions");

        if (unlockpt(fdm_) == -1) throw std::runtime_error("failed to unlock slave");

        slave_name_ = [&]() {
            const auto* slave_name = ptsname(fdm_);
            if (!slave_name) throw std::runtime_error("failed to acquire slave name");
            return slave_name;
        }();

        std::ofstream ofs(pts_file_, std::ofstream::trunc);
        if (ofs.fail()) throw std::runtime_error(std::string("failed to open ") + pts_file_);
        ofs << slave_name_;
        ofs.close();
    }

    ~Term()
    {
        std::remove(pts_file_);
        if (fdm_ != -1) close(fdm_);
    }

    static void AtExit() { std::remove(pts_file_); }

    std::string GetName() const { return slave_name_; }

    std::vector<std::string> ReadLines()
    {
        while (true) {
            std::array<char, 512> buf;
            auto len = ::read(fdm_, buf.data(), buf.size());
            if (len <= 0) {
                head_.clear();
                return {};
            }

            auto cmds = ranges::actions::split(std::string(buf.data(), len), '\r');
            cmds[0] = head_ + cmds[0];
            if (buf[len - 1] != '\r') {
                head_ = cmds.back();
                cmds.pop_back();
            } else {
                head_.clear();
            }
            if (!cmds.empty()) return cmds;
        }
    }

    void Write(std::string msg) const
    {
        if (msg.empty()) return;

        if (!boost::ends_with(msg, "\n")) msg += "\n";
        msg = boost::regex_replace(msg, boost::regex{R"(\n)"}, "\r\n");
        ::write(fdm_, msg.c_str(), msg.size());
    }

private:
    static const constexpr auto pts_file_ = "/tmp/master.pts";
    int fdm_ = -1;
    std::string slave_name_;
    std::string head_;
};

constexpr const auto welcome = R"(
╦ ╦┌─┐┬  ┬  ┌─┐┌─┐┌┬┐┌─┐┬
║║║├┤ │  │  │  │ ││││├┤ │
╚╩╝└─┘┴─┘┴─┘└─┘└─┘┴ ┴└─┘o

Press ['ctrl+a', '\'] to exit screen session.

)";

int main()
try {
    std::signal(SIGINT, [](int) {
        Term::AtExit();
        exit(1);
    });

    std::unique_ptr<Term> term;
    while (true) {
        if (!term) {
            term = std::make_unique<Term>();
            std::cout << "start listening at " << term->GetName() << std::endl;
        }

        auto cmds = term->ReadLines();
        if (cmds.empty()) {
            std::cout << "all clients disconnected\n";
            term.reset();
            continue;
        }
        for (const auto& cmd : cmds) {
            std::cout << "recv: " << cmd << std::endl;

            if (cmd == "hello") {
                term->Write(welcome);
            } else {
                term->Write(std::move(cmd));
            }
        }
    }
    return 0;
} catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
}
