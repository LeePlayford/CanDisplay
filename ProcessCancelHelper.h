#ifndef PROCESSCANCELHELPER_H
#define PROCESSCANCELHELPER_H

#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <QDebug>

class ProcessCancelHelper {
private:
    pid_t process_id_;
    pid_t process_id_1;

public:
    ProcessCancelHelper()
        : process_id_(0) {}

    int StartProcess(const char* command_buffer) {
        const char* command_argv[4];
        command_argv[0] = "sh";
        command_argv[1] = "-c";
        command_argv[2] = command_buffer;
        command_argv[3] = nullptr;

        process_id_ = fork();
        if (process_id_ == -1) return -1;
        if (process_id_ == 0) {
            process_id_1 = execvp(command_argv[0], const_cast<char* const*>(command_argv));
            //assert(false && "execvp did not work");
            //_exit(-1);
        }

        /*int wait_status;
        if (waitpid(process_id_, &wait_status, 0))
        {
            process_id_ = 0;
            if (WIFEXITED(wait_status)) return (WEXITSTATUS(wait_status));
            return -1;
        }*/

        qDebug() << process_id_;
        qDebug() << process_id_1;
        return process_id_;
    }

    bool CancelProcess()
    {
        if (process_id_ != 0)
        {
            kill(process_id_, SIGKILL);
            system ("pkill candump");
        }
        process_id_ = 0;
        return true;
    }
};


#endif // PROCESSCANCELHELPER_H
