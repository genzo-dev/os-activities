#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>

constexpr unsigned SLEEP_RED    = 3;
constexpr unsigned SLEEP_YELLOW = 2;
constexpr unsigned SLEEP_GREEN  = 4;

static void safe_write(const char* msg) {
    ::write(STDOUT_FILENO, msg, std::strlen(msg));  
}

static pid_t g_child_pid = 0;

/* ---------------------- HANDLERS DO FILHO ---------------------- */
extern "C" void child_handle_red(int) {
    safe_write("FILHO: VERMELHO\n");
}

extern "C" void child_handle_yellow(int) {
    safe_write("FILHO: AMARELO\n");
}

extern "C" void child_handle_green(int) {
    safe_write("FILHO: VERDE\n");
}

extern "C" void child_handle_terminate(int) {
    safe_write("FILHO: Recebeu SIGTSTP -> finalizando.\n");
    _exit(0);
}

/* ---------------------- HANDLERS DO PAI ----------------------- */
extern "C" void parent_handle_sigint(int) {

    safe_write("PAI: Recebeu SIGINT (Ctrl+C) -> enviando VERMELHO ao filho.\n");
    if (g_child_pid > 0) {
        kill(g_child_pid, SIGUSR1);
    }

}

extern "C" void parent_handle_sigtstp(int) {
    safe_write("PAI: Recebeu SIGTSTP (Ctrl+Z) -> finalizando ambos.\n");
    if (g_child_pid > 0) {
        kill(g_child_pid, SIGTERM);
        waitpid(g_child_pid, nullptr, 0);
    }
    safe_write("PAI: Saindo agora.\n");
    _exit(0);
}

extern "C" void parent_handle_alarm(int) {
    static int state = 0;
    state = (state + 1) % 3;

    if (g_child_pid <= 0) return;

    if (state == 0) { 
        safe_write("PAI(handler): Troca -> VERMELHO (SIGUSR1 enviado ao filho)\n");
        kill(g_child_pid, SIGUSR1);
        alarm(SLEEP_RED);
    } else if (state == 1) { 
        safe_write("PAI(handler): Troca -> VERDE (SIGALRM enviado ao filho)\n");
        kill(g_child_pid, SIGALRM);
        alarm(SLEEP_GREEN);
    } else {
        safe_write("PAI(handler): Troca -> AMARELO (SIGUSR2 enviado ao filho)\n");
        kill(g_child_pid, SIGUSR2);
        alarm(SLEEP_YELLOW);
    }
}

/* ---------------------- MAIN ---------------------- */
int main() {
    pid_t pid = fork();
    if (pid < 0) {
        const char* err = "Erro: fork() falhou\n";
        safe_write(err);
        return 1;
    }

    if (pid == 0) {
        // =========== PROCESSO FILHO ===========

        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);

        sa.sa_handler = child_handle_red;
        if (sigaction(SIGUSR1, &sa, nullptr) < 0) { safe_write("Erro sigaction SIGUSR1\n"); _exit(1); }

        sa.sa_handler = child_handle_red;
        if (sigaction(SIGINT, &sa, nullptr) < 0) { safe_write("Erro sigaction SIGINT\n"); _exit(1); }

        // SIGUSR2 -> AMARELO
        sa.sa_handler = child_handle_yellow;
        if (sigaction(SIGUSR2, &sa, nullptr) < 0) { safe_write("Erro sigaction SIGUSR2\n"); _exit(1); }

        sa.sa_handler = child_handle_green;
        if (sigaction(SIGALRM, &sa, nullptr) < 0) { safe_write("Erro sigaction SIGALRM\n"); _exit(1); }

        sa.sa_handler = child_handle_terminate;
        if (sigaction(SIGTSTP, &sa, nullptr) < 0) { safe_write("Erro sigaction SIGTSTP\n"); _exit(1); }

        safe_write("FILHO: iniciado. Aguardando sinais...\n");

        while (true) {
            pause();
        }

        _exit(0);
    } else {
        // =========== PROCESSO PAI ===========
        g_child_pid = pid;

        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);


        sa.sa_handler = parent_handle_sigint;
        if (sigaction(SIGINT, &sa, nullptr) < 0) { safe_write("Erro sigaction pai SIGINT\n"); return 1; }

        sa.sa_handler = parent_handle_sigtstp;
        if (sigaction(SIGTSTP, &sa, nullptr) < 0) { safe_write("Erro sigaction pai SIGTSTP\n"); return 1; }

        sa.sa_handler = parent_handle_alarm;
        if (sigaction(SIGALRM, &sa, nullptr) < 0) { safe_write("Erro sigaction pai SIGALRM\n"); return 1; }

        safe_write("PAI: iniciado. Estado inicial -> VERMELHO. Enviando SIGUSR1 ao filho.\n");
        kill(g_child_pid, SIGUSR1);
        alarm(SLEEP_RED);

        while (true) {
            pause();
        }

        return 0;
    }
}
