#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "hash_utils.h"

#define RESULT_FILE "password_found.txt"
#define PROGRESS_INTERVAL 100000

// Incrementa a senha como contador
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    for (int i = password_len - 1; i >= 0; i--) {
        int pos = -1;
        for (int j = 0; j < charset_len; j++) {
            if (password[i] == charset[j]) {
                pos = j;
                break;
            }
        }
        if (pos == -1) return 0;

        if (pos + 1 < charset_len) {
            password[i] = charset[pos + 1];
            return 1;
        } else {
            password[i] = charset[0];
        }
    }
    return 0;
}

int password_compare(const char *a, const char *b) {
    return strcmp(a, b);
}

int check_result_exists() {
    return access(RESULT_FILE, F_OK) == 0;
}

void save_result(int worker_id, const char *password) {
    int fd = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        char buffer[128];
        int len = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
        write(fd, buffer, len);
        close(fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Uso interno: %s <hash> <start> <end> <charset> <len> <id>\n", argv[0]);
        return 1;
    }

    const char *target_hash = argv[1];
    char *start_password = argv[2];
    const char *end_password = argv[3];
    const char *charset = argv[4];
    int password_len = atoi(argv[5]);
    int worker_id = atoi(argv[6]);
    int charset_len = strlen(charset);

    printf("[Worker %d] Iniciado: %s até %s\n", worker_id, start_password, end_password);

    char current_password[11];
    strcpy(current_password, start_password);

    char computed_hash[33];
    long long passwords_checked = 0;
    time_t start_time = time(NULL);

    while (1) {
        // a cada N senhas, verificar se outro já encontrou
        if (passwords_checked % PROGRESS_INTERVAL == 0) {
            if (check_result_exists()) {
                printf("[Worker %d] Encerrando, outro encontrou.\n", worker_id);
                break;
            }
        }

        // calcular hash
        md5_string(current_password, computed_hash);

        // comparar
        if (strcmp(computed_hash, target_hash) == 0) {
            printf("[Worker %d] SENHA ENCONTRADA: %s\n", worker_id, current_password);
            save_result(worker_id, current_password);
            break;
        }

        // fim do intervalo?
        if (password_compare(current_password, end_password) == 0) {
            break;
        }

        // incrementar
        if (!increment_password(current_password, charset, charset_len, password_len)) {
            break;
        }

        passwords_checked++;
    }

    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);

    printf("[Worker %d] Finalizado. Total: %lld senhas em %.2f segundos", 
           worker_id, passwords_checked, total_time);
    if (total_time > 0) {
        printf(" (%.0f senhas/s)", passwords_checked / total_time);
    }
    printf("\n");

    return 0;
}

