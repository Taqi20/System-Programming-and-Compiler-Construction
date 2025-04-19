#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSYM 1000

typedef struct
{
    char symbol[32];
    int address;
} Sym_Entry;

Sym_Entry symtab[MAXSYM];
int symCount = 0;

int lookup_sym(const char *sym)
{
    for (int i = 0; i < symCount; i++)
    {
        if (strcmp(symtab[i].symbol, sym) == 0)
            return symtab[i].address;
    }
    printf("Error: undefined symbol %s\n", sym);
    exit(1);
}

int main()
{
    FILE *intf = fopen("intermediate.dat", "r");
    FILE *objf = fopen("objcode.dat", "w");
    if (!intf || !objf)
    {
        perror("File open error");
        return 1;
    }

    char label[32], opcode[16], operand[64];
    int LC;

    FILE *temp = fopen("intermediate.dat", "r");
    while (fscanf(temp, "%X %s %s %[^\n]", &LC, label, opcode, operand) == 4)
    {
        if (strcmp(label, "**") != 0 && strcmp(opcode, "END") != 0)
            symtab[symCount++] = (Sym_Entry){.symbol = "", .address = LC},
            strcpy(symtab[symCount - 1].symbol, label);
    }
    fclose(temp);

    while (fscanf(intf, "%X %s %s %[^\n]", &LC, label, opcode, operand) == 4)
    {
        if (strcmp(opcode, "START") == 0)
        {
            fprintf(objf, "H^%s^%06X\n", label, LC);
        }
        else if (strcmp(opcode, "END") == 0)
        {
            fprintf(objf, "E^%06X\n", lookup_sym(label));
        }
        else if (strcmp(opcode, "DC") == 0)
        {
            fprintf(objf, "T^%06X^04^0000000%s\n", LC, operand);
        }
        else
        {
            int reg = 0;
            char sym[32];
            sscanf(operand, "R%d,%s", &reg, sym);
            int addr = lookup_sym(sym);
            int opcode_bin = 0x58;
            unsigned char obj[4] = {
                (unsigned char)opcode_bin,
                (unsigned char)(reg << 4),
                (unsigned char)((addr >> 8) & 0xFF),
                (unsigned char)(addr & 0xFF)};
            fprintf(objf, "T^%06X^04^%02X%02X%02X%02X\n",
                    LC, obj[0], obj[1], obj[2], obj[3]);
        }
    }

    fclose(intf);
    fclose(objf);

    printf("Object code generated in objcode.dat\n");
    return 0;
}
