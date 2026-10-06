/*
    UNIVERSIDADE FEDERAL DO PARANA
    SETOR DE EDUCACAO PROFISSIONAL E TECNOLOGICA
    TECNOLOGIA EM ANALISE E DESENVOLVIMENTO DE SISTEMAS
    DISCIPLINA: ESTRUTURA DE DADOS I
    PROFESSORA: DRA. ANDREIA DE JESUS

    1o TRABALHO PRATICO - JOGO DA VELHA

    OSMAR NUNES JUNIOR            GRR20250999
    CEZAR ANTHONIO LIMA LLHEO     GRR20256527

    IMPLEMENTACAO:
    - Listas lineares encadeadas (jogadas, partidas e ranking)
    - Minimax com poda alfa-beta (jogadas do computador)
    - Persistencia em partidas_velha.txt (modo append)
    - Programa modular, sem variaveis globais

    REFERENCIA DO ALGORITMO:
    UC Berkeley - CS 188 - Minimax e Alpha-Beta Pruning
    https://inst.eecs.berkeley.edu/~cs188/textbook/games/minimax.html

    FORMATO DE CADA LINHA DO ARQUIVO (campos separados por ';'):
    ID;usuario;jogada1;...;computador;jogada1;...;resultado
    Cada jogada tem o formato linha-coluna (ex.: 1-2).

    COMPILACAO:
    gcc -Wall -Wextra -std=c99 -o jogovelha jogovelha.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define TAM 3
#define MAX_NOME 100
#define MAX_LINHA 1000
#define MAX_CAMPOS 32
#define ALFA_INICIAL -1000
#define BETA_INICIAL 1000
#define ARQUIVO "partidas_velha.txt"
#define NOME_COMPUTADOR "Computador"
#define TEXTO_EMPATE "EMPATE"

typedef struct Jogada {
    int linha, coluna;
    struct Jogada *prox;
} Jogada;

typedef struct Partida {
    int id, salva;
    char usuario[MAX_NOME], computador[MAX_NOME], resultado[MAX_NOME];
    char simboloUsuario, simboloComputador;
    Jogada *jogadasUsuario, *jogadasComputador;
    struct Partida *prox;
} Partida;

typedef struct Ranking {
    char nome[MAX_NOME];
    int vitorias;
    struct Ranking *prox;
} Ranking;


/* ======================= UTILITARIOS / ENTRADA ======================= */

void *alocar(size_t bytes) {
    void *p = malloc(bytes);

    if (p == NULL) {
        printf("Erro de memoria.\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

/* Le uma linha (descarta o excesso). Fim de entrada encerra o programa. */
void lerTexto(char *texto, int tamanho) {
    int c;

    if (fgets(texto, tamanho, stdin) == NULL) {
        printf("\nEntrada encerrada. Programa finalizado.\n");
        exit(EXIT_SUCCESS);
    }
    if (strchr(texto, '\n') == NULL) {
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    texto[strcspn(texto, "\r\n")] = '\0';
}

int lerInteiro(void) {
    char linha[MAX_NOME], *fim;
    long v;

    while (1) {
        lerTexto(linha, MAX_NOME);
        v = strtol(linha, &fim, 10);

        if (fim != linha && v >= -1000000 && v <= 1000000) {
            while (isspace((unsigned char)*fim)) {
                fim++;
            }
            if (*fim == '\0') {
                return (int)v;
            }
        }
        printf("Digite um numero valido: ");
    }
}

/* Le "linha coluna" na mesma linha (ex.: 2 3). */
void lerJogada(int *linha, int *coluna) {
    char texto[MAX_NOME], sobra[2];

    while (1) {
        lerTexto(texto, MAX_NOME);
        if (sscanf(texto, "%d %d %1s", linha, coluna, sobra) == 2) {
            return;
        }
        printf("Digite linha e coluna separadas por espaco (ex.: 2 3): ");
    }
}

int lerSimNao(const char *mensagem) {
    char texto[MAX_NOME], *p;

    while (1) {
        printf("%s", mensagem);
        lerTexto(texto, MAX_NOME);

        p = texto;
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (tolower((unsigned char)*p) == 's') {
            return 1;
        }
        if (tolower((unsigned char)*p) == 'n') {
            return 0;
        }
        printf("Responda com s ou n.\n");
    }
}

int igual(const char *a, const char *b) {   /* ignora maiusculas */
    for (; *a && *b; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
    }
    return *a == *b;
}

void aparar(char *texto) {                   /* remove espacos das pontas */
    char *ini = texto;
    size_t n;

    while (isspace((unsigned char)*ini)) {
        ini++;
    }
    n = strlen(ini);
    while (n > 0 && isspace((unsigned char)ini[n - 1])) {
        n--;
    }
    memmove(texto, ini, n);
    texto[n] = '\0';
}

int pareceJogada(const char *t) {            /* formato "1-2" */
    return strlen(t) == 3 && t[0] >= '1' && t[0] <= '3' &&
           t[1] == '-' && t[2] >= '1' && t[2] <= '3';
}

/* Um nome que lembre jogada, "EMPATE" ou o computador quebraria a
   leitura do arquivo e o ranking. */
int nomeValido(const char *nome) {
    return *nome != '\0' && strchr(nome, ';') == NULL &&
           !igual(nome, TEXTO_EMPATE) && !igual(nome, NOME_COMPUTADOR) &&
           !pareceJogada(nome);
}

void lerNomeUsuario(char *usuario) {
    while (1) {
        printf("Nome do usuario: ");
        lerTexto(usuario, MAX_NOME);
        aparar(usuario);

        if (nomeValido(usuario)) {
            return;
        }
        printf("Nome invalido. Nao pode ser vazio, conter ';', ser \"%s\" "
               "ou \"%s\", nem ter formato de jogada (ex.: 1-2).\n",
               TEXTO_EMPATE, NOME_COMPUTADOR);
    }
}


/* ======================= LISTAS: JOGADAS E PARTIDAS ======================= */

void adicionarJogada(Jogada **lista, int linha, int coluna) {
    Jogada *nova = alocar(sizeof(Jogada));

    nova->linha = linha;
    nova->coluna = coluna;
    nova->prox = NULL;

    while (*lista != NULL) {
        lista = &(*lista)->prox;
    }
    *lista = nova;
}

void liberarJogadas(Jogada *lista) {
    Jogada *aux;

    while (lista != NULL) {
        aux = lista;
        lista = lista->prox;
        free(aux);
    }
}

Partida *criarPartida(int id, const char *usuario) {
    Partida *nova = alocar(sizeof(Partida));

    memset(nova, 0, sizeof(Partida));
    nova->id = id;
    snprintf(nova->usuario, MAX_NOME, "%s", usuario);
    snprintf(nova->computador, MAX_NOME, "%s", NOME_COMPUTADOR);
    snprintf(nova->resultado, MAX_NOME, "%s", TEXTO_EMPATE);
    return nova;
}

void adicionarPartida(Partida **lista, Partida *nova) {
    while (*lista != NULL) {
        lista = &(*lista)->prox;
    }
    *lista = nova;
}

void liberarPartidas(Partida *lista) {
    Partida *aux;

    while (lista != NULL) {
        aux = lista;
        lista = lista->prox;
        liberarJogadas(aux->jogadasUsuario);
        liberarJogadas(aux->jogadasComputador);
        free(aux);
    }
}

int existePartidaNaoSalva(Partida *lista) {
    for (; lista != NULL; lista = lista->prox) {
        if (!lista->salva) {
            return 1;
        }
    }
    return 0;
}


/* ======================= TABULEIRO ======================= */

void mostrarTabuleiro(char t[TAM][TAM]) {
    int i;

    printf("\n       1   2   3\n");
    for (i = 0; i < TAM; i++) {
        printf("   %d   %c | %c | %c \n", i + 1, t[i][0], t[i][1], t[i][2]);
        if (i < TAM - 1) {
            printf("      ---+---+---\n");
        }
    }
    printf("\n");
}

int venceu(char t[TAM][TAM], char s) {
    int i;

    for (i = 0; i < TAM; i++) {
        if ((t[i][0] == s && t[i][1] == s && t[i][2] == s) ||
            (t[0][i] == s && t[1][i] == s && t[2][i] == s)) {
            return 1;
        }
    }
    return t[1][1] == s && ((t[0][0] == s && t[2][2] == s) ||
                            (t[0][2] == s && t[2][0] == s));
}

int tabuleiroCheio(char t[TAM][TAM]) {
    int i;

    for (i = 0; i < TAM * TAM; i++) {
        if (t[i / TAM][i % TAM] == ' ') {
            return 0;
        }
    }
    return 1;
}


/* ======================= MINIMAX COM PODA ALFA-BETA =======================
   Vitoria do computador = 10 - profundidade (prefere ganhar rapido);
   vitoria do usuario = profundidade - 10; empate = 0.
   O computador maximiza e o usuario minimiza. Quando alfa >= beta, os
   ramos restantes nao podem alterar o resultado e sao podados. */

int minimax(char t[TAM][TAM], char pc, char us,
            int prof, int maximizando, int alfa, int beta) {
    int i, valor;
    int melhor = maximizando ? ALFA_INICIAL : BETA_INICIAL;
    char *celula;

    if (venceu(t, pc)) {
        return 10 - prof;
    }
    if (venceu(t, us)) {
        return prof - 10;
    }
    if (tabuleiroCheio(t)) {
        return 0;
    }

    for (i = 0; i < TAM * TAM && alfa < beta; i++) {
        celula = &t[i / TAM][i % TAM];
        if (*celula != ' ') {
            continue;
        }

        *celula = maximizando ? pc : us;
        valor = minimax(t, pc, us, prof + 1, !maximizando, alfa, beta);
        *celula = ' ';

        if (maximizando) {
            if (valor > melhor) melhor = valor;
            if (melhor > alfa) alfa = melhor;
        } else {
            if (valor < melhor) melhor = valor;
            if (melhor < beta) beta = melhor;
        }
    }
    return melhor;
}

/* Retorna a celula (0..8) da melhor jogada. Se varias tem o mesmo valor
   otimo, sorteia uma (partidas variadas sem perder a otimalidade). */
int melhorJogada(char t[TAM][TAM], char pc, char us) {
    int i, valor, melhor = ALFA_INICIAL, empatadas = 0, escolhida = -1;
    char *celula;

    for (i = 0; i < TAM * TAM; i++) {
        celula = &t[i / TAM][i % TAM];
        if (*celula != ' ') {
            continue;
        }

        *celula = pc;
        valor = minimax(t, pc, us, 0, 0, ALFA_INICIAL, BETA_INICIAL);
        *celula = ' ';

        if (valor > melhor) {
            melhor = valor;
            empatadas = 1;
            escolhida = i;
        } else if (valor == melhor && rand() % ++empatadas == 0) {
            escolhida = i;
        }
    }
    return escolhida;
}


/* ======================= PARTIDA ======================= */

void jogarPartida(Partida *p, int usuarioComeca) {
    char t[TAM][TAM];
    char su = usuarioComeca ? 'X' : 'O';
    char sc = usuarioComeca ? 'O' : 'X';
    int vezUsuario = usuarioComeca, n, l, c, k;

    p->simboloUsuario = su;
    p->simboloComputador = sc;
    memset(t, ' ', sizeof(t));

    for (n = 0; n < TAM * TAM; n++) {
        mostrarTabuleiro(t);

        if (vezUsuario) {
            printf("Sua jogada (linha coluna): ");
            while (1) {
                lerJogada(&l, &c);
                if (l >= 1 && l <= TAM && c >= 1 && c <= TAM &&
                    t[l - 1][c - 1] == ' ') {
                    break;
                }
                printf("Jogada invalida. Tente novamente: ");
            }
            t[l - 1][c - 1] = su;
            adicionarJogada(&p->jogadasUsuario, l, c);
        } else {
            k = melhorJogada(t, sc, su);
            l = k / TAM + 1;
            c = k % TAM + 1;
            t[l - 1][c - 1] = sc;
            printf("Computador jogou: %d %d\n", l, c);
            adicionarJogada(&p->jogadasComputador, l, c);
        }

        if (venceu(t, vezUsuario ? su : sc)) {
            snprintf(p->resultado, MAX_NOME, "%s",
                     vezUsuario ? p->usuario : p->computador);
            mostrarTabuleiro(t);
            printf("Resultado: %s venceu!\n", p->resultado);
            return;
        }
        vezUsuario = !vezUsuario;
    }

    mostrarTabuleiro(t);                 /* resultado ja e EMPATE */
    printf("Resultado: %s\n", TEXTO_EMPATE);
}


/* ======================= HISTORICO ======================= */

void mostrarJogadas(Jogada *lista) {
    for (; lista != NULL; lista = lista->prox) {
        printf("%d-%d%s", lista->linha, lista->coluna,
               lista->prox != NULL ? ", " : "");
    }
}

/* Historico das partidas de 'ini' ate o fim da lista (conjunto jogado
   agora) e o vencedor geral desse conjunto. */
void mostrarHistorico(Partida *ini) {
    Partida *p;
    int vu = 0, vc = 0, empates = 0;

    printf("\n========================================\n");
    printf("          HISTORICO DAS PARTIDAS\n");
    printf("========================================\n");

    for (p = ini; p != NULL; p = p->prox) {
        printf("\nPartida %d\n", p->id);

        if (strcmp(p->resultado, TEXTO_EMPATE) == 0) {
            printf("Resultado: EMPATE\nJogadores: %s e %s\n",
                   p->usuario, p->computador);
            empates++;
        } else {
            printf("Vencedor: %s\n", p->resultado);
            if (strcmp(p->resultado, p->usuario) == 0) {
                vu++;
            } else {
                vc++;
            }
        }

        printf("Usuario (%s, %c): ", p->usuario, p->simboloUsuario);
        mostrarJogadas(p->jogadasUsuario);
        printf("\nComputador (%s, %c): ", p->computador, p->simboloComputador);
        mostrarJogadas(p->jogadasComputador);
        printf("\n");
    }

    printf("\n========================================\n");
    printf("Placar: %s %d x %d %s (empates: %d)\n",
           ini->usuario, vu, vc, ini->computador, empates);
    printf("VENCEDOR GERAL: %s\n",
           vu > vc ? ini->usuario : vc > vu ? ini->computador : "EMPATE");
    printf("========================================\n");
}


/* ======================= ARQUIVO ======================= */

void gravarJogadas(FILE *f, Jogada *j) {
    for (; j != NULL; j = j->prox) {
        fprintf(f, ";%d-%d", j->linha, j->coluna);
    }
}

void salvarPartidas(Partida *lista) {
    FILE *f;
    Partida *p;
    int quantidade = 0;

    if (!existePartidaNaoSalva(lista)) {
        printf("Nao ha partidas novas para salvar.\n");
        return;
    }

    f = fopen(ARQUIVO, "a");
    if (f == NULL) {
        printf("Erro ao abrir %s.\n", ARQUIVO);
        return;
    }

    for (p = lista; p != NULL; p = p->prox) {
        if (p->salva) {
            continue;
        }
        fprintf(f, "%d;%s", p->id, p->usuario);
        gravarJogadas(f, p->jogadasUsuario);
        fprintf(f, ";%s", p->computador);
        gravarJogadas(f, p->jogadasComputador);
        fprintf(f, ";%s\n", p->resultado);

        p->salva = 1;
        quantidade++;
    }

    fclose(f);
    printf("%d partida(s) salva(s) com sucesso em %s.\n", quantidade, ARQUIVO);
}

/* Separa a linha em campos por ';' (altera a linha). Retorna o numero de
   campos, ou 0 se a linha for vazia ou tiver campos demais. */
int separarCampos(char *s, char *campos[], int max) {
    int n = 0;

    s[strcspn(s, "\r\n")] = '\0';
    if (*s == '\0') {
        return 0;
    }

    campos[n++] = s;
    for (; *s != '\0'; s++) {
        if (*s == ';') {
            *s = '\0';
            if (n >= max) {
                return 0;
            }
            campos[n++] = s + 1;
        }
    }
    return n;
}

/* Le o proximo registro valido do arquivo; retorna o numero de campos
   ou 0 no fim do arquivo. */
int lerRegistro(FILE *f, char *linha, char *campos[]) {
    int n;

    while (fgets(linha, MAX_LINHA, f) != NULL) {
        if ((n = separarCampos(linha, campos, MAX_CAMPOS)) > 0) {
            return n;
        }
    }
    return 0;
}

/* O proximo ID e o maior ID do arquivo + 1 (IDs unicos entre sessoes). */
int obterProximoId(void) {
    FILE *f = fopen(ARQUIVO, "r");
    char linha[MAX_LINHA], *campos[MAX_CAMPOS];
    int id, maior = 0;

    if (f == NULL) {
        return 1;
    }
    while (lerRegistro(f, linha, campos) > 0) {
        id = atoi(campos[0]);
        if (id > maior) {
            maior = id;
        }
    }
    fclose(f);
    return maior + 1;
}


/* ======================= RANKING ======================= */

Ranking *buscarOuCriar(Ranking **lista, const char *nome) {
    Ranking *r;

    for (r = *lista; r != NULL; r = r->prox) {
        if (igual(r->nome, nome)) {
            return r;
        }
    }

    r = alocar(sizeof(Ranking));
    snprintf(r->nome, MAX_NOME, "%s", nome);
    r->vitorias = 0;
    r->prox = *lista;
    *lista = r;
    return r;
}

/* Ordena por vitorias (decrescente); empate: ordem alfabetica. */
void ordenarRanking(Ranking *lista) {
    Ranking *i, *j;
    int v;
    char nome[MAX_NOME];

    for (i = lista; i != NULL; i = i->prox) {
        for (j = i->prox; j != NULL; j = j->prox) {
            if (j->vitorias > i->vitorias ||
                (j->vitorias == i->vitorias &&
                 strcmp(j->nome, i->nome) < 0)) {
                v = i->vitorias;
                i->vitorias = j->vitorias;
                j->vitorias = v;
                strcpy(nome, i->nome);
                strcpy(i->nome, j->nome);
                strcpy(j->nome, nome);
            }
        }
    }
}

void liberarRanking(Ranking *lista) {
    Ranking *aux;

    while (lista != NULL) {
        aux = lista;
        lista = lista->prox;
        free(aux);
    }
}

/* Layout: id;usuario;jogadas...;computador;jogadas...;resultado */
void processarRegistro(Ranking **lista, char *campos[], int n) {
    int i = 2;
    Ranking *ru, *rc;

    if (n < 4) {
        return;
    }
    while (i < n && pareceJogada(campos[i])) {
        i++;
    }
    if (i >= n - 1) {
        return;                          /* registro mal formado */
    }

    ru = buscarOuCriar(lista, campos[1]);
    rc = buscarOuCriar(lista, campos[i]);

    if (igual(campos[n - 1], campos[1])) {
        ru->vitorias++;
    } else if (igual(campos[n - 1], campos[i])) {
        rc->vitorias++;
    }
}

void exibirRanking(void) {
    FILE *f = fopen(ARQUIVO, "r");
    char linha[MAX_LINHA], *campos[MAX_CAMPOS];
    int n, posicao = 1;
    Ranking *lista = NULL, *r;

    if (f == NULL) {
        printf("Arquivo %s ainda nao existe. Salve partidas primeiro.\n",
               ARQUIVO);
        return;
    }
    while ((n = lerRegistro(f, linha, campos)) > 0) {
        processarRegistro(&lista, campos, n);
    }
    fclose(f);

    ordenarRanking(lista);

    printf("\n========================================\n");
    printf("          RANKING DE VITORIAS\n");
    printf("========================================\n");
    if (lista == NULL) {
        printf("Nenhuma partida registrada.\n");
    }
    for (r = lista; r != NULL; r = r->prox) {
        printf("%d. %s - %d vitoria(s)\n", posicao++, r->nome, r->vitorias);
    }
    printf("========================================\n");

    liberarRanking(lista);
}


/* ======================= JOGAR PARTIDAS ======================= */

/* Par ou impar: se a soma bater com a escolha do usuario, ele comeca
   (joga com X). Retorna 1 se o usuario comeca. */
int decidirPrimeiroJogador(const char *usuario) {
    int escolha, numUsuario, numComputador, soma;

    do {
        printf("%s, escolha par ou impar (0=par, 1=impar): ", usuario);
        escolha = lerInteiro();
    } while (escolha != 0 && escolha != 1);

    do {
        printf("Digite um numero de 0 a 10: ");
        numUsuario = lerInteiro();
    } while (numUsuario < 0 || numUsuario > 10);

    numComputador = rand() % 11;
    soma = numUsuario + numComputador;
    printf("O computador escolheu %d. Soma = %d (%s).\n",
           numComputador, soma, soma % 2 ? "impar" : "par");

    return soma % 2 == escolha;
}

void jogarPartidas(Partida **lista, int *proximoId) {
    char usuario[MAX_NOME];
    int comeca = 0, primeira = 1;
    Partida *p, *inicio = NULL;

    lerNomeUsuario(usuario);

    do {
        comeca = primeira ? decidirPrimeiroJogador(usuario) : !comeca;
        primeira = 0;

        printf("\n%s comeca jogando com X.\n",
               comeca ? usuario : NOME_COMPUTADOR);

        p = criarPartida((*proximoId)++, usuario);
        adicionarPartida(lista, p);
        if (inicio == NULL) {
            inicio = p;
        }

        printf("--- Partida %d ---\n", p->id);
        jogarPartida(p, comeca);

    } while (lerSimNao("\nJogar outra partida? (s/n): "));

    mostrarHistorico(inicio);
}


/* ======================= MAIN ======================= */

int main(void) {
    Partida *partidas = NULL;
    int opcao, proximoId;

    srand((unsigned int)time(NULL));
    proximoId = obterProximoId();

    do {
        printf("\n========================================\n");
        printf("             JOGO DA VELHA\n");
        printf("========================================\n");
        printf("1) Jogar partidas de Jogo da Velha\n");
        printf("2) Salvar as partidas do Jogo da Velha\n");
        printf("3) Ranquear os usuarios do Jogo da Velha\n");
        printf("4) Sair do Jogo da Velha\n");
        printf("========================================\n");
        printf("Opcao: ");

        opcao = lerInteiro();

        switch (opcao) {
            case 1:
                jogarPartidas(&partidas, &proximoId);
                break;
            case 2:
                salvarPartidas(partidas);
                break;
            case 3:
                exibirRanking();
                break;
            case 4:
                if (existePartidaNaoSalva(partidas) &&
                    lerSimNao("Deseja salvar as partidas da sessao "
                              "atual? (s/n): ")) {
                    salvarPartidas(partidas);
                }
                printf("Programa encerrado.\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }
    } while (opcao != 4);

    liberarPartidas(partidas);
    return 0;
}