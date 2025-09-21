# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Andre kim (10226152) 
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

[Dividi o espaco de busca de senhas em faixas lexicograficas]

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
// Cole seu código de divisão aqui

for (int i = 0; i < num_workers; i++) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Erro ao criar worker");
        exit(1);
    } else if (pid == 0) {
        // Processo filho -> vira um worker
        char start[10], end[10];
        // calcula os limites do intervalo
        snprintf(start, sizeof(start), "%c%s", 'a' + i, "aa");
        snprintf(end, sizeof(end), "%c%s", 'a' + i, "zz");

        execl("./worker", "worker", hash, start, end, charset, argv[2], NULL);

        perror("Erro no execl");
        exit(1);
    } else {
        // Processo pai guarda PID do filho
        worker_pids[i] = pid;
        printf("[coord] forkou worker %d (pid=%d) faixa %c***\n", 
               i, pid, 'a' + i);
    }
}

```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

[O fork() para criar processor, execl() para substituir cada filho pelo programa .worker passando argumentos e wait() para esperar todos terminarem antes de o coordinator ler o arquivo]

**Código do fork/exec:**
```c
// Cole aqui seu loop de criação de workers
pid_t worker_pids[num_workers];

for (int i = 0; i < num_workers; i++) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Erro ao criar worker");
        exit(1);
    } else if (pid == 0) {
        // Processo filho -> substitui por ./worker
        char worker_id[16], tamanho_str[16];
        snprintf(worker_id, sizeof(worker_id), "%d", i);
        snprintf(tamanho_str, sizeof(tamanho_str), "%d", plen);

        execl("./worker", "worker", hash, start[i], end[i], charset, tamanho_str, worker_id, NULL);

        // Só chega aqui se execl falhar
        perror("Erro no execl");
        exit(1);
    } else {
        // Processo pai armazena o PID do filho
        worker_pids[i] = pid;
        printf("[coord] forkou worker %d (pid=%d)\n", i, pid);
    }
}

// Pai aguarda todos os filhos
for (int i = 0; i < num_workers; i++) {
    int status;
    waitpid(worker_pids[i], &status, 0);
    if (WIFEXITED(status)) {
        printf("Worker %d terminou com código %d\n", i, WEXITSTATUS(status));
    }
}

```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**

[Cada worker tenta criar password_found.txt com flags 0_CREAT|0_EXCL, garantindo escrita atomica, apenas o primeiro consegue os outros trabalhadores saem sem sobrescrever]
Leia sobre condições de corrida (aqui)[https://pt.stackoverflow.com/questions/159342/o-que-%C3%A9-uma-condi%C3%A7%C3%A3o-de-corrida]

**Como o coordinator consegue ler o resultado?**

[o coordinator abre esse arquivo no final , faz parse da string worker_i e senha e imprime o resultado]

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | 0.961s | 0.953s | 1.057s | ≈ 0.91x |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | 11.917s | 9.862s | 10.343s | ≈ 1.15x |

**O speedup foi linear? Por quê?**
[Não. Observamos que o aumento de workers não trouxe ganho proporcional. Custos de criação de processos (fork/exec), sincronização e verificação do arquivo de resultado introduzem overheads que, para esses tamanhos de problema, reduzem a vantagem da paralelização.]

---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
[Maior desafio foi implementar corretamente o incremento lexicográfico do espaço de busca para charsets e tamanhos arbitrários; garantir limites (start/end) corretos. FOi resolvido modelando a senha como vetor de índices no charset e incremento por carry (como contador) ]

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [x] Código compila sem erros
- [x] Todos os TODOs foram implementados
- [x] Testes passam no `./tests/simple_test.sh`
- [x] Relatório preenchido
