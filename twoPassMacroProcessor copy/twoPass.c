#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 100
#define MAX_MNT_ENTRIES 10
#define MAX_MDT_ENTRIES 100
#define MAX_ALA_ENTRIES 10

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct
{
    char name[20];
    int mdt_index;
} MNTEntry;

typedef struct
{
    char args[MAX_ALA_ENTRIES][20];
    int count;
} ALA;

MNTEntry MNT[MAX_MNT_ENTRIES];
char MDT[MAX_MDT_ENTRIES][MAX_LINE_LENGTH];
int MNTC = 0, MDTC = 0;

void pass1(FILE *source, FILE *intermediate);
void pass2(FILE *intermediate, FILE *expanded);
void substitute_args(char *line, ALA *call_ala);

int main(void)
{
    FILE *source = fopen("source.asm", "r");
    if (!source)
    {
        perror("Error opening source.asm");
        exit(1);
    }
    FILE *intermediate = fopen("intermediate.txt", "w+");
    if (!intermediate)
    {
        perror("Error opening intermediate.txt");
        exit(1);
    }
    FILE *expanded = fopen("expanded.asm", "w");
    if (!expanded)
    {
        perror("Error opening expanded.asm");
        exit(1);
    }

    pass1(source, intermediate);
    rewind(intermediate);
    pass2(intermediate, expanded);

    fclose(source);
    fclose(intermediate);
    fclose(expanded);

    printf("Macro processing complete. Check expanded.asm\n");
    return 0;
}

void pass1(FILE *source, FILE *intermediate)
{
    char line[MAX_LINE_LENGTH];
    int inside_macro = 0;

    while (fgets(line, MAX_LINE_LENGTH, source))
    {
        if (strstr(line, "MACRO") != NULL)
        {
            inside_macro = 1;
            if (fgets(line, MAX_LINE_LENGTH, source))
            {
                char temp[MAX_LINE_LENGTH];
                strcpy(temp, line);
                char *token = strtok(temp, " \t\n");
                if (token != NULL)
                {
                    if (token[0] == '&')
                    {
                        token = strtok(NULL, " \t\n");
                    }
                    strcpy(MNT[MNTC].name, token);
                    MNT[MNTC].mdt_index = MDTC;
                    MNTC++;
                }
                strcpy(MDT[MDTC++], line);
            }
        }
        else if (inside_macro)
        {
            if (strstr(line, "MEND") != NULL)
            {
                strcpy(MDT[MDTC++], line);
                inside_macro = 0;
            }
            else
            {
                strcpy(MDT[MDTC++], line);
            }
        }
        else
        {
            fputs(line, intermediate);
        }
    }
}

void substitute_args(char *line, ALA *call_ala)
{
    for (int i = 0; i < call_ala->count; i++)
    {
        char argToken[20];
        sprintf(argToken, "&ARG%d", i + 1);
        char *pos = strstr(line, argToken);
        while (pos != NULL)
        {
            char temp[MAX_LINE_LENGTH];
            int index = pos - line;
            line[index] = '\0';
            strcpy(temp, line);
            strcat(temp, call_ala->args[i]);
            strcat(temp, pos + strlen(argToken));
            strcpy(line, temp);
            pos = strstr(line, argToken);
        }
    }
}

void pass2(FILE *intermediate, FILE *expanded)
{
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, intermediate))
    {
        char copy[MAX_LINE_LENGTH];
        strcpy(copy, line);

        char *token1 = strtok(copy, " \t\n");
        char *token2 = strtok(NULL, " \t\n");

        char macroName[20] = "";
        int labelPresent = 0;
        int found = 0;

        if (token1 != NULL)
        {
            for (int i = 0; i < MNTC; i++)
            {
                if (strcmp(token1, MNT[i].name) == 0)
                {
                    strcpy(macroName, token1);
                    found = 1;
                    break;
                }
            }
            if (!found && token2 != NULL)
            {
                strcpy(macroName, token2);
                labelPresent = 1;
            }
        }

        int is_macro = 0;
        int macro_index = -1;
        if (strlen(macroName) > 0)
        {
            for (int i = 0; i < MNTC; i++)
            {
                if (strcmp(macroName, MNT[i].name) == 0)
                {
                    is_macro = 1;
                    macro_index = i;
                    break;
                }
            }
        }

        if (is_macro)
        {
            ALA call_ala;
            call_ala.count = 0;
            char *rest;
            rest = strtok(NULL, "\n");
            if (rest != NULL)
            {
                while (*rest == ' ' || *rest == '\t')
                    rest++;
                char *arg = strtok(rest, ",");
                while (arg != NULL)
                {
                    while (*arg == ' ' || *arg == '\t')
                        arg++;
                    char *end = arg + strlen(arg) - 1;
                    while (end > arg && (*end == ' ' || *end == '\t'))
                    {
                        *end = '\0';
                        end--;
                    }
                    strcpy(call_ala.args[call_ala.count], arg);
                    call_ala.count++;
                    arg = strtok(NULL, ",");
                }
            }
            for (int j = MNT[macro_index].mdt_index; j < MDTC; j++)
            {
                if (strstr(MDT[j], "MEND") != NULL)
                    break;
                char expanded_line[MAX_LINE_LENGTH];
                strcpy(expanded_line, MDT[j]);
                substitute_args(expanded_line, &call_ala);
                fprintf(expanded, "%s", expanded_line);
            }
        }
        else
        {
            fprintf(expanded, "%s", line);
        }
    }
}
