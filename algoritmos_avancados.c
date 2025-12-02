#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NOME 60
#define MAX_PISTA 120
#define HASH_TABLE_SIZE 101  // tamanho para poucas pistas

// Estruturas principais

// Sala: nó da árvore binária que representa cada cômodo
typedef struct sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA];      // pista associada à sala (string vazia se nenhuma)
    struct sala *esquerda;
    struct sala *direita;
} Sala;

// PistaNode: nó da BST que guarda as pistas coletadas (únicas)
typedef struct pistaNode {
    char *pista;                // alocada dinamicamente
    struct pistaNode *esq;
    struct pistaNode *dir;
} PistaNode;

// HashEntry: item da tabela hash — mapeia pista -> suspeito
typedef struct hashEntry {
    char *chave;                // pista (alocada)
    char *suspeito;             // nome do suspeito (alocado)
    struct hashEntry *proximo;  // encadeamento separado (chaining)
} HashEntry;

// HashTable: array de ponteiros para HashEntry
typedef struct {
    HashEntry *tabela[HASH_TABLE_SIZE];
} HashTable;

// Funções utilitárias de string
static char *strdup_s(const char *s) {
    if (!s) return NULL;
    char *c = malloc(strlen(s) + 1);
    if (!c) {
        fprintf(stderr, "Erro: sem memória (strdup_s)\n");
        exit(EXIT_FAILURE);
    }
    strcpy(c, s);
    return c;
}

static void toLowerInPlace(char *s) {
    for (; *s; ++s) *s = (char) tolower((unsigned char) *s);
}

// Comparação de strings case-insensitive (retorna 0 se iguais)
static int strcmpIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

// criarSala()
// Cria dinamicamente uma sala com nome e pista opcional.
Sala* criarSala(const char *nome, const char *pista) {
    Sala *nova = (Sala*) malloc(sizeof(Sala));
    if (!nova) {
        fprintf(stderr, "Erro: sem memória para criar sala.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(nova->nome, nome, MAX_NOME-1);
    nova->nome[MAX_NOME-1] = '\0';

    if (pista && pista[0] != '\0') {
        strncpy(nova->pista, pista, MAX_PISTA-1);
        nova->pista[MAX_PISTA-1] = '\0';
    } else {
        nova->pista[0] = '\0';
    }

    nova->esquerda = nova->direita = NULL;
    return nova;
}

// BST de Pistas: inserirPista() e criação de nó
// Inserimos apenas pistas não vazias e sem duplicatas.
PistaNode* criarPistaNode(const char *texto) {
    PistaNode *n = (PistaNode*) malloc(sizeof(PistaNode));
    if (!n) {
        fprintf(stderr, "Erro: sem memória para nó de pista.\n");
        exit(EXIT_FAILURE);
    }
    n->pista = strdup_s(texto);
    n->esq = n->dir = NULL;
    return n;
}

PistaNode* inserirPistaRec(PistaNode *raiz, const char *texto) {
    if (!texto || texto[0] == '\0') return raiz;
    if (raiz == NULL) return criarPistaNode(texto);

    int cmp = strcmp(texto, raiz->pista);
    if (cmp < 0) {
        raiz->esq = inserirPistaRec(raiz->esq, texto);
    } else if (cmp > 0) {
        raiz->dir = inserirPistaRec(raiz->dir, texto);
    } else {
        // duplicata -> não inserir
    }
    return raiz;
}

// Função pública que modifica a raiz (passada por referência)
void inserirPista(PistaNode **raizRef, const char *texto) {
    if (!texto || texto[0] == '\0') return;
    *raizRef = inserirPistaRec(*raizRef, texto);
}

// exibirPistas()
// Imprime as pistas coletadas em ordem alfabética (in-order).
void exibirPistasRec(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistasRec(raiz->esq);
    printf("- %s\n", raiz->pista);
    exibirPistasRec(raiz->dir);
}

void exibirPistas(PistaNode *raiz) {
    if (!raiz) {
        printf("Nenhuma pista coletada.\n");
        return;
    }
    printf("\nPistas coletadas (ordem alfabética):\n");
    exibirPistasRec(raiz);
}

// Hash table simples (djb2) para mapear pista -> suspeito

// djb2 hash
static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char) *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

// Inicializa a tabela com NULLs
void iniciarHashTable(HashTable *ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) ht->tabela[i] = NULL;
}

// Insere um par (pista -> suspeito). Faz strdup nas strings.
void inserirHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!pista || pista[0] == '\0') return;
    unsigned long h = hash_djb2(pista) % HASH_TABLE_SIZE;
    HashEntry *entry = (HashEntry*) malloc(sizeof(HashEntry));
    if (!entry) {
        fprintf(stderr, "Erro: sem memória para hash entry.\n");
        exit(EXIT_FAILURE);
    }
    entry->chave = strdup_s(pista);
    entry->suspeito = strdup_s(suspeito);
    entry->proximo = ht->tabela[h];
    ht->tabela[h] = entry;
}

// Busca suspeito pelo texto da pista. Retorna NULL se não encontrado.
const char *buscarHash(HashTable *ht, const char *pista) {
    if (!pista) return NULL;
    unsigned long h = hash_djb2(pista) % HASH_TABLE_SIZE;
    HashEntry *e = ht->tabela[h];
    while (e) {
        if (strcmp(e->chave, pista) == 0) return e->suspeito;
        e = e->proximo;
    }
    return NULL;
}

// Libera toda a tabela hash
void liberarHashTable(HashTable *ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        HashEntry *e = ht->tabela[i];
        while (e) {
            HashEntry *next = e->proximo;
            free(e->chave);
            free(e->suspeito);
            free(e);
            e = next;
        }
        ht->tabela[i] = NULL;
    }
}

// -------------------------------------------------------
// explorarSalasComPistas()
// Navegação interativa pela mansão. Ao entrar numa sala,
// exibe a pista (se houver) e a adiciona na BST de pistas.
// Opções: 'e' esquerda, 'd' direita, 's' sair.
// -------------------------------------------------------
void explorarSalasComPistas(Sala *inicio, PistaNode **arvorePistas) {
    if (!inicio) return;
    Sala *atual = inicio;
    char opcao;

    while (1) {
        printf("\n== Você está em: %s ==\n", atual->nome);

        if (strlen(atual->pista) > 0) {
            printf("Pista encontrada: \"%s\"\n", atual->pista);
            inserirPista(arvorePistas, atual->pista);
            // Para que a mesma pista não seja coletada várias vezes,
            // limpa a pista da sala após coletá-la.
            atual->pista[0] = '\0';
        } else {
            printf("Nenhuma pista nova aqui.\n");
        }

        // Mostrar opçõe
        printf("\nOpções:\n");
        if (atual->esquerda) printf("  (e) Ir para a esquerda -> %s\n", atual->esquerda->nome);
        if (atual->direita)  printf("  (d) Ir para a direita  -> %s\n", atual->direita->nome);
        printf("  (s) Sair e acusar um suspeito\n");
        printf("Escolha: ");

        if (scanf(" %c", &opcao) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("Entrada inválida. Tente novamente.\n");
            continue;
        }
        opcao = (char) tolower((unsigned char) opcao);

        if (opcao == 'e') {
            if (atual->esquerda) atual = atual->esquerda;
            else printf("Não há caminho à esquerda.\n");
        } else if (opcao == 'd') {
            if (atual->direita) atual = atual->direita;
            else printf("Não há caminho à direita.\n");
        } else if (opcao == 's') {
            printf("Você escolheu encerrar a investigação.\n");
            return;
        } else {
            printf("Opção inválida. Use 'e', 'd' ou 's'.\n");
        }
    }
}

// Contagem de quantas pistas coletadas apontam para um suspeito
// Percorre a BST de pistas coletadas e, para cada pista,
// consulta a hash table para obter o suspeito.
int contarPistasParaSuspeitoRec(PistaNode *raiz, HashTable *ht, const char *suspeitoAlvo) {
    if (!raiz) return 0;
    int count = 0;
    // esquerda
    count += contarPistasParaSuspeitoRec(raiz->esq, ht, suspeitoAlvo);
    // atual
    const char *sus = buscarHash(ht, raiz->pista);
    if (sus != NULL && strcmpIgnoreCase(sus, suspeitoAlvo) == 0) {
        count++;
    }
    // direita
    count += contarPistasParaSuspeitoRec(raiz->dir, ht, suspeitoAlvo);
    return count;
}

int contarPistasParaSuspeito(PistaNode *raiz, HashTable *ht, const char *suspeitoAlvo) {
    if (!raiz || !suspeitoAlvo) return 0;
    return contarPistasParaSuspeitoRec(raiz, ht, suspeitoAlvo);
}

// Função para imprimir cada pista coletada e o suspeito associado (se houver)
void mostrarPistasComSuspeitosRec(PistaNode *raiz, HashTable *ht) {
    if (!raiz) return;
    mostrarPistasComSuspeitosRec(raiz->esq, ht);
    const char *sus = buscarHash(ht, raiz->pista);
    if (sus) printf("- %s  -> aponta para: %s\n", raiz->pista, sus);
    else     printf("- %s  -> aponta para: (desconhecido)\n", raiz->pista);
    mostrarPistasComSuspeitosRec(raiz->dir, ht);
}

// Funções de liberação de memória
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz->pista);
    free(raiz);
}

void liberarSalas(Sala *raiz) {
    if (!raiz) return;
    liberarSalas(raiz->esquerda);
    liberarSalas(raiz->direita);
    free(raiz);
}

// main()
// Monta o mapa fixo da mansão, popula a tabela hash (pista->suspeito),
// permite a exploração e no final solicita a acusação.
int main(void) {
    // Montagem do mapa (mapa simples)

    Sala *hall = criarSala("Hall de Entrada", "Bilhete com endereço");
    Sala *salaEstar = criarSala("Sala de Estar", "Pegadas no tapete");
    Sala *cozinha = criarSala("Cozinha", "Faca com manchas");
    Sala *jardim = criarSala("Jardim", "Folha rasgada");
    Sala *biblioteca = criarSala("Biblioteca", "Livro deslocado");

    hall->esquerda = salaEstar;
    hall->direita = cozinha;
    salaEstar->esquerda = jardim;
    cozinha->direita = biblioteca;

    // Preparar a BST de pistas coletadas (vazia)
    PistaNode *arvorePistas = NULL;

    // Popula a tabela hash com regras: pista -> suspeito
    // (Regra codificada manualmente: não há entrada do usuário)
    HashTable ht;
    iniciarHashTable(&ht);

    inserirHash(&ht, "Bilhete com endereço", "Sr. Almeida");
    inserirHash(&ht, "Pegadas no tapete", "Sra. Beatriz");
    inserirHash(&ht, "Faca com manchas", "Carlos");
    inserirHash(&ht, "Folha rasgada", "Sra. Beatriz");
    inserirHash(&ht, "Livro deslocado", "Carlos");

    // Mensagem inicial
    printf("=== MANSÃO: Investigação Final ===\n");
    printf("Explore a mansão e colete pistas.\n");
    printf("Navegue com: (e) esquerda, (d) direita, (s) sair e acusar.\n");

    // Exploração interativa (coleta automática de pistas)
    explorarSalasComPistas(hall, &arvorePistas);

    // pistas coletadas e com quem elas se relacionam
    if (!arvorePistas) {
        printf("\nVoce não coletou nenhuma pista. Impossível acusar.\n");
    } else {
        printf("\n--- Pistas coletadas e seus suspeitos ---\n");
        mostrarPistasComSuspeitosRec(arvorePistas, &ht);

        // Pedir ao jogador que acuse um suspeito
        char acusacao[80];
        int c;
        // consumir novo-line pendente
        while ((c = getchar()) != '\n' && c != EOF) {}
        printf("\nDigite o nome do suspeito que deseja acusar: ");
        if (!fgets(acusacao, sizeof(acusacao), stdin)) {
            printf("Entrada inválida.\n");
            // liberar memórias antes de sair
            liberarSalas(hall);
            liberarPistas(arvorePistas);
            liberarHashTable(&ht);
            return 0;
        }
        // remover \n final
        size_t len = strlen(acusacao);
        if (len > 0 && acusacao[len-1] == '\n') acusacao[len-1] = '\0';

        // contar quantas pistas apontam para o acusado
        int total = contarPistasParaSuspeito(arvorePistas, &ht, acusacao);

        printf("\nVocê acusou: %s\n", acusacao);
        printf("Pistas que apontam para %s: %d\n", acusacao, total);

        if (total >= 2) {
            printf("\nResultado: ACUSACAO SUSTENTADA.\n");
            printf("Há evidências suficientes para responsabilizar %s.\n", acusacao);
        } else {
            printf("\nResultado: ACUSACAO NÃO SUSTENTADA.\n");
            printf("Pelo menos 2 pistas são necessárias para sustentar a acusação.\n");
        }
    }

    // Liberar memória
    liberarSalas(hall);
    liberarPistas(arvorePistas);
    liberarHashTable(&ht);

    printf("\nInvestigação encerrada. Obrigado por jogar!\n");
    return 0;
}