#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[])
{
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-C handler");
    }

    // TODO: setup sig alarm handler
    struct sigaction input_for_sigaction;
    sigemptyset(&input_for_sigaction.sa_mask);
    input_for_sigaction.sa_flags = SA_RESTART;
    input_for_sigaction.sa_handler = alarmHandler;
    if(sigaction(SIGALRM, &input_for_sigaction, nullptr) == -1){
        perror("smash error: failed to set alarm handler");
    }
    SmallShell &smash_inst = SmallShell::getInstance();
    while (true)
    {
        // smash_inst.jobs_list->removeFinishedJobs_(smash_inst.fg_job_pid);
        std::cout << smash_inst.curr_prompt;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash_inst.jobs_list->removeFinishedJobs();
        smash_inst.executeCommand(cmd_line.c_str());
    }
    if (kill(smash_inst.smash_pid, SIGKILL) == -1)
    {
        perror("smash error: kill failed");
    }
    exit(0);
}