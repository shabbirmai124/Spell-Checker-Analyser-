#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#define MAX_WORD_LENGTH 50
#define HASH_TABLE_SIZE 10007

// Global accumulators in milliseconds
double lcs_total = 0;
double string_match_total = 0;
double knapsack_total = 0;

typedef struct WordNode {
    char word[MAX_WORD_LENGTH];
    struct WordNode *next;
} WordNode;
WordNode *dictionary[HASH_TABLE_SIZE] = {NULL};

typedef struct {
    char word[MAX_WORD_LENGTH];
    int profit;
    int weight;
} Candidate;

// Hash function remains unchanged
unsigned int hash(const char *str) {
    unsigned long hash = 5381;
    while (*str) hash = ((hash << 5) + hash) + *(str++);
    return hash % HASH_TABLE_SIZE;
}

void filterWord(char *word) {
    int i = 0, j = 0;
    while (word[i] != '\0') {
        if (isalpha(word[i])) {
            word[j++] = tolower(word[i]);
        }
        i++;
    }
    word[j] = '\0';
}

void addToDictionary(const char *word) {
    char lowerWord[MAX_WORD_LENGTH];
    strncpy(lowerWord, word, MAX_WORD_LENGTH - 1);
    lowerWord[MAX_WORD_LENGTH - 1] = '\0';
    filterWord(lowerWord);

    unsigned int index = hash(lowerWord);
    WordNode *newNode = (WordNode *)malloc(sizeof(WordNode));
    if (!newNode) exit(1);

    strcpy(newNode->word, lowerWord);
    newNode->next = dictionary[index];
    dictionary[index] = newNode;
}

// Helper function: compute elapsed time in milliseconds
double diff_in_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int stringMatch(const char *text, const char *pattern) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int result = 0;
    int n = strlen(text);
    int m = strlen(pattern);
    for (int i = 0; i <= n - m; i++) {
        int j;
        for (j = 0; j < m; j++) {
            if (text[i + j] != pattern[j])
                break;
        }
        if (j == m) {
            result = 1;
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    string_match_total += diff_in_ms(start, end);
    return result;
}

int isCorrectWord(const char *word) {
    char lowerWord[MAX_WORD_LENGTH];
    strncpy(lowerWord, word, MAX_WORD_LENGTH - 1);
    lowerWord[MAX_WORD_LENGTH - 1] = '\0';
    filterWord(lowerWord);

    WordNode *current = dictionary[hash(lowerWord)];
    while (current) {
        if (!strcmp(current->word, lowerWord) || stringMatch(current->word, lowerWord))
            return 1;
        current = current->next;
    }
    return 0;
}

int LCS(const char *s1, const char *s2) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int dp[len1 + 1][len2 + 1];

    for (int i = 0; i <= len1; i++) {
        for (int j = 0; j <= len2; j++) {
            if (i == 0 || j == 0)
                dp[i][j] = 0;
            else if (s1[i - 1] == s2[j - 1])
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = (dp[i - 1][j] > dp[i][j - 1]) ? dp[i - 1][j] : dp[i][j - 1];
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    lcs_total += diff_in_ms(start, end);
    return dp[len1][len2];
}

void suggestCorrections(const char *word, FILE *outputFile) {
    char lowerWord[MAX_WORD_LENGTH];
    strncpy(lowerWord, word, MAX_WORD_LENGTH - 1);
    lowerWord[MAX_WORD_LENGTH - 1] = '\0';
    filterWord(lowerWord);

    int len_word = strlen(lowerWord);
    int capacity = 3;
    char firstLetter = len_word > 0 ? lowerWord[0] : '\0';
    char lastLetter = len_word > 0 ? lowerWord[len_word - 1] : '\0';

    Candidate candidates[1000];
    int numCandidates = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WordNode *current = dictionary[i];
        while (current != NULL) {
            int dictWordLength = strlen(current->word);
            if (dictWordLength >= 1 &&
                current->word[0] == firstLetter &&
                current->word[dictWordLength - 1] == lastLetter &&
                (dictWordLength == len_word || dictWordLength == len_word + 1)) {

                if (numCandidates < 1000) {
                    strcpy(candidates[numCandidates].word, current->word);
                    candidates[numCandidates].profit = LCS(lowerWord, current->word);
                    candidates[numCandidates].weight = 1;
                    numCandidates++;
                }
            }
            current = current->next;
        }
    }

    struct timespec start_knapsack, end_knapsack;
    clock_gettime(CLOCK_MONOTONIC, &start_knapsack);

    int max_items = 3;
    int dp[max_items + 1][capacity + 1];
    memset(dp, -1, sizeof(dp));
    dp[0][0] = 0;

    for (int i = 0; i < numCandidates; i++) {
        int p = candidates[i].profit;
        int w = candidates[i].weight;

        for (int k = max_items; k >= 1; k--) {
            for (int j = capacity; j >= w; j--) {
                if (dp[k - 1][j - w] != -1) {
                    if (dp[k][j] < dp[k - 1][j - w] + p) {
                        dp[k][j] = dp[k - 1][j - w] + p;
                    }
                }
            }
        }
    }

    int max_profit = -1;
    int best_k = 0, best_w = 0;

    for (int k = 0; k <= max_items; k++) {
        for (int j = 0; j <= capacity; j++) {
            if (dp[k][j] > max_profit) {
                max_profit = dp[k][j];
                best_k = k;
                best_w = j;
            }
        }
    }

    Candidate selected[3];
    int selectedCount = 0;
    int remaining_k = best_k;
    int remaining_w = best_w;

    for (int i = numCandidates - 1; i >= 0 && remaining_k > 0; i--) {
        int p = candidates[i].profit;
        int w = candidates[i].weight;

        if (w <= remaining_w && dp[remaining_k][remaining_w] == dp[remaining_k - 1][remaining_w - w] + p) {
            selected[selectedCount++] = candidates[i];
            remaining_k--;
            remaining_w -= w;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_knapsack);
    knapsack_total += diff_in_ms(start_knapsack, end_knapsack);

    fprintf(outputFile, "\n\nIncorrect Word: '%s'\n", word);
    fprintf(outputFile, "Suggested Correct Words:");
    if (selectedCount == 0) {
        fprintf(outputFile, " No suitable suggestions found for '%s'.\n", word);
        return;
    }

    for (int i = 0; i < selectedCount; i++) {
        fprintf(outputFile, " '%s'", selected[i].word);
    }
    fprintf(outputFile, "\n");
}

void loadDictionaryFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening dictionary file.\n");
        exit(1);
    }
    char word[MAX_WORD_LENGTH];
    while (fscanf(file, "%s", word) == 1) {
        addToDictionary(word);
    }
    fclose(file);
}

void processWordsFromFile(const char *filename, int limit, FILE *outputFile, int *totalWords, int *correct, int *incorrect) {
    *correct = 0;
    *incorrect = 0;
    *totalWords = 0;

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening words file.\n");
        exit(1);
    }

    char word[MAX_WORD_LENGTH];
    int wordCount = 0;

    while (fscanf(file, "%s", word) == 1 && wordCount < limit) {
        filterWord(word);

        if (isCorrectWord(word)) {
            (*correct)++;
        } else {
            (*incorrect)++;
            suggestCorrections(word, outputFile);
        }
        wordCount++;
    }

    *totalWords = wordCount;

    fclose(file);
}

int main() {
    loadDictionaryFromFile("Dictionary.txt");
    FILE *csv;

    csv = fopen("total_time.csv", "w");
    fprintf(csv, "Words,Time (s)\n");
    fclose(csv);

    csv = fopen("lcs_time.csv", "w");
    fprintf(csv, "Words,Time (ms)\n");
    fclose(csv);

    csv = fopen("string_match_time.csv", "w");
    fprintf(csv, "Words,Time (ms)\n");
    fclose(csv);

    csv = fopen("knapsack_time.csv", "w");
    fprintf(csv, "Words,Time (ms)\n");
    fclose(csv);

    printf("\n\n\t\t\t===========================================================================\n");
    printf("\t\t\t\t\t\tSpell Checker and Analyser\n");
    printf("\t\t\t===========================================================================\n\n\n");

    for (int i = 1; i <= 10; i++) {
        int current_limit = i * 1000;
        printf("\n\tProcessing %d words...\n", current_limit);
        lcs_total = 0;
        string_match_total = 0;
        knapsack_total = 0;

        FILE *outputFile = fopen("Output.txt", "w");
        if (!outputFile) {
            fprintf(stderr, "Error opening output file for %d words.\n", current_limit);
            exit(1);
        }

        int totalWords, correct, incorrect;
        struct timespec start_total, end_total;
        clock_gettime(CLOCK_MONOTONIC, &start_total);
        processWordsFromFile("Input.txt", current_limit, outputFile, &totalWords, &correct, &incorrect);
        clock_gettime(CLOCK_MONOTONIC, &end_total);

        double total_time = (end_total.tv_sec - start_total.tv_sec) + (end_total.tv_nsec - start_total.tv_nsec) / 1e9;
        double lcs_time = lcs_total;
        double string_time = string_match_total;
        double knapsack_time = knapsack_total;

        fprintf(outputFile, "\n\n\n\n\nResults for %d words:\n\t", current_limit);
        fprintf(outputFile, "Total Words Processed: %d\n\t", totalWords);
        fprintf(outputFile, "Correct Words: %d\n\t", correct);
        fprintf(outputFile, "Incorrect Words: %d\n", incorrect);
        fprintf(outputFile, "\nRuntime Statistics:\n\t");
        fprintf(outputFile, "Total processing time: %.4f seconds\n\t", total_time);
        fprintf(outputFile, "LCS computation time: %.4f milliseconds\n\t", lcs_time);
        fprintf(outputFile, "String matching time: %.4f milliseconds\n\t", string_time);
        fprintf(outputFile, "Knapsack algorithm time: %.4f milliseconds\n\t", knapsack_time);

        fclose(outputFile);

        csv = fopen("total_time.csv", "a");
        fprintf(csv, "%d,%.4f\n", current_limit, total_time);
        fclose(csv);

        csv = fopen("lcs_time.csv", "a");
        fprintf(csv, "%d,%.4f\n", current_limit, lcs_time);
        fclose(csv);

        csv = fopen("string_match_time.csv", "a");
        fprintf(csv, "%d,%.4f\n", current_limit, string_time);
        fclose(csv);

        csv = fopen("knapsack_time.csv", "a");
        fprintf(csv, "%d,%.4f\n", current_limit, knapsack_time);
        fclose(csv);

        printf("\n\tResults for %d words:\n\t\t", current_limit);
        printf("Total Words Processed: %d\n\t\t", totalWords);
        printf("Correct Words: %d\n\t\t", correct);
        printf("Incorrect Words: %d\n\n\t", incorrect);
        printf("Runtime Statistics:\n\t\t");
        printf("Total processing time: %.4f seconds\n\t\t", total_time);
        printf("LCS computation time: %.4f milliseconds\n\t\t", lcs_time);
        printf("String matching time: %.4f milliseconds\n\t\t", string_time);
        printf("Knapsack algorithm time: %.4f milliseconds\n\n\n\t\t", knapsack_time);
    }

    return 0;
}
