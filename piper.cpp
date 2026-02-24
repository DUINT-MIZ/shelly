#include <unordered_map>
#include <list> 
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <utility>
#include <cstring>
#include <system_error>
#include <iostream>

struct string_hash {
	using is_transparent = void;

	auto operator()(std::string_view sv) const {
		return std::hash<std::string_view>{}(sv);
	}
};

void sys_err(const char* msg) {
	throw std::system_error(errno, std::generic_category(), msg);
}

void sys_err(const std::string& msg) {
	throw std::system_error(errno, std::generic_category(), msg);
}

class Piping {
	private :
	int pipefd[2]{-1, -1};
	
	static void close_fd(int& fd) noexcept {
		if(fd != -1)
			close(fd);
		fd = -1;
	}

	static void close_fd(const int& fd) noexcept {
		if(fd != -1)
			close(fd);
	}

	public :
	Piping() noexcept { new_pipe(); }

	Piping(const Piping& _) = delete;
	Piping& operator=(const Piping& _) = delete;

	Piping(Piping&& oth) noexcept {
		pipefd[0] = std::exchange(oth.pipefd[0], -1);
		pipefd[1] = std::exchange(oth.pipefd[1], -1);
	}

	Piping& operator=(Piping&& oth) noexcept {
		if(this == &oth) return *this;
		close_fd(pipefd[0]);
		close_fd(pipefd[1]);
		pipefd[0] = std::exchange(oth.pipefd[0], -1);
		pipefd[1] = std::exchange(oth.pipefd[1], -1);
		return *this;
	}

	int read_end() const noexcept { return pipefd[0]; }
	int write_end() const noexcept { return pipefd[1]; }

	void new_pipe() {
		close_pipe();
		if(pipe(pipefd) == -1) {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Piping : new_pipe() failed"
			);
		}
	}

	static int equalize(int targetfd, int sourcefd) noexcept {
		return dup2(sourcefd, targetfd);
	}

	static int duplicate(int targetfd) noexcept {
		return dup(targetfd);
	}

	void close_pipe() noexcept {
		close_fd(pipefd[0]);
		close_fd(pipefd[1]);
	}

	void cclose_pipe() const noexcept {
		close_fd(pipefd[0]);
		close_fd(pipefd[1]);
	}
	
	~Piping() noexcept {
		close_pipe();
	}
};

void panic(const char* msg) {
    perror(msg);
    exit(1);
}

void run_pipe(std::vector<char**>& commands) {
    int real_stdin = Piping::duplicate(STDIN_FILENO);
    if(real_stdin == -1) sys_err("run_pipe : duplicate() failed");
	Piping curr_pipe;
	curr_pipe.close_pipe();
	int i = 0;
	while(i < commands.size()) {
		std::cerr << "i : " << i << "\n";
		bool not_end = (i + 1 < commands.size());

		if(not_end) { 
			curr_pipe.new_pipe();
		}

		pid_t pid = fork();
		if(pid == -1) sys_err("fork failed");
		if(pid == 0) {
			if(not_end) {
				Piping::equalize(STDOUT_FILENO, curr_pipe.write_end());
				curr_pipe.cclose_pipe();
			}

			close(real_stdin);
			std::cerr << "Executed : " << *commands[i] << std::endl;
			execv(*commands[i], commands[i]);
			sys_err("execv failed");
		} else {
			if(not_end) {
				Piping::equalize(STDIN_FILENO, curr_pipe.read_end());
				curr_pipe.close_pipe();
			}
		}
	}

	Piping::equalize(STDIN_FILENO, real_stdin);
	while(wait(NULL) > 0);
}

int main(int argc, char** argv) {
	std::vector<char**> vec;
	int left = 1;
	for(int i = 1; i <= argc; i++) {
		if(!argv[i] or (strcmp(argv[i], "/") == 0)) {
			vec.push_back(&argv[left]);
			argv[i] = nullptr;
			left = i;
			++left;
		}
	}
	
	for(auto cmd : vec) {
		std::cout << "args : ";
		while(*cmd) {
			std::cout << "\"" << *cmd << "\" ";
			++cmd;
		}
		std::cout << std::endl;
	}
	
	run_pipe(vec);
}



