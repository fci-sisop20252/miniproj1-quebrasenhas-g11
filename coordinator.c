// coordinator.c
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define RESULT_FILE "password_found.txt"
#define WORKER_PATH "./worker"

static bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static void make_range(char *start, char *end,
                       char first, const char *charset, int len) {
    // start = first + (charset[0] repetido)
    // end   = first + (charset[last] repetido)
    int last = (int)strlen(charset) - 1;
    start[0] = first;
    end[0]   = first;
    for (int i = 1; i < len; i++) {
        start[i] = charset[0];
        end[i]   = charset[last];
    }
    start[len] = '\0';
    end[len]   = '\0';
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
            "Uso: %s <hash_md5> <charset> <tamanho>\n"
            "Ex.: %s 900150983cd24fb0d6963f7d28e17f72 abc 3\n",
            argv[0], argv[0]);
        return 1;
    }

    const char *hash = argv[1];
    const char *charset = argv[2];
    int plen = atoi(argv[3]);

    int nworkers = (int)strlen(charset);
    if (nworkers <= 0 || plen <= 0) {
        fprintf(stderr, "Erro: charset vazio ou tamanho inválido.\n");
        return 1;
    }

    // limpa resultado anterior
    unlink(RESULT_FILE);

    pid_t *pids = calloc(nworkers, sizeof(pid_t));
    if (!pids) { perror("calloc"); return 1; }

    // cria todos os filhos
    for (int i = 0; i < nworkers; i++) {
        char start[256], end[256];
        make_range(start, end, charset[i], charset, plen);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // em caso de falha, tente matar já criados
            for (int k = 0; k < i; k++) if (pids[k] > 0) kill(pids[k], SIGTERM);
            free(pids);
            return 1;
        }
        if (pid == 0) {
            // filho -> exec worker
            char worker_id[16], tamanho_str[16];
            snprintf(worker_id, sizeof(worker_id), "%d", i);
            snprintf(tamanho_str, sizeof(tamanho_str), "%d", plen);

            // argv do worker: ./worker <hash> <inicio> <fim> <charset> <tamanho> <worker_id>
            char *wargv[] = {
                (char *)"worker",
                (char *)hash,
                start,
                end,
                (char *)charset,
                tamanho_str,
                worker_id,
                NULL
            };
            execv(WORKER_PATH, wargv);
            perror("execv");
            _exit(127);
        } else {
            pids[i] = pid;
            fprintf(stdout, "[coord] forkou worker %d (pid=%d) faixa %c***\n", i, pid, charset[i]);
        }
    }

    // loop de monitoramento: para assim que houver RESULT_FILE
    int vivos = nworkers;
    while (vivos > 0) {
        if (file_exists(RESULT_FILE)) {
            // achou: encerra os que restam
            for (int i = 0; i < nworkers; i++) {
                if (pids[i] > 0) kill(pids[i], SIGTERM);
            }
            break;
        }
        // coleta não bloqueante
        int status;
        pid_t r = waitpid(-1, &status, WNOHANG);
        if (r > 0) {
            vivos--;
        } else {
            usleep(20000); // 20 ms
        }
    }

    // garante que todos saíram
    for (int i = 0; i < nworkers; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }

    // mostra resultado (se existir)
    if (file_exists(RESULT_FILE)) {
        FILE *f = fopen(RESULT_FILE, "r");
        if (f) {
            char buf[512];
            if (fgets(buf, sizeof(buf), f)) {
                // formato esperado: "<worker_id>:<senha>\n"
                printf("[coord] Resultado: %s", buf);
            }
            fclose(f);
        } else {
            perror("fopen resultado");
        }
    } else {
        printf("[coord] Senha não encontrada nas faixas geradas.\n");
    }

    free(pids);
    return 0;
}
