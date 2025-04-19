#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSYM 1000
#define MAXLIT 1000

typedef struct
{
    char mnemonic[8];
    int length;
} MOT_Entry;

typedef struct
{
    char symbol[32];
    int address;
} Sym_Entry;

typedef struct
{
    char literal[16];
    int address;
} Lit_Entry;

MOT_Entry MOT[] = {
    // --- RR format (2 bytes) ---------------------------------------
    {"SPM", 2},   // Set Program Mask
    {"BALR", 2},  // Branch And Link Register
    {"BCTR", 2},  // Branch on Count Register
    {"BCR", 2},   // Branch on Condition Register
    {"NOPR", 2},  // No Operation Register (BC with mask 0)
    {"BR", 2},    // Branch Register (Unconditional)
    {"SSK", 2},   // Set Storage Key
    {"ISK", 2},   // Insert Storage Key
    {"BSM", 2},   // Branch and Set Mode
    {"BASSM", 2}, // Branch and Save and Set Mode
    {"BASR", 2},  // Branch And Save Register
    {"LPR", 2},   // Load Positive Register
    {"LNR", 2},   // Load Negative Register
    {"LTR", 2},   // Load and Test Register
    {"LCR", 2},   // Load and Complement Register
    {"NR", 2},    // AND Register
    {"CLR", 2},   // Compare Logical Register
    {"OR", 2},    // Or Register
    {"XR", 2},    // Exclusive‑or Register
    {"LR", 2},    // Load Register
    {"CR", 2},    // Compare Register
    {"AR", 2},    // Add Register
    {"SR", 2},    // Subtract Register
    {"MR", 2},    // Multiply Register
    {"DR", 2},    // Divide Register
    {"ALR", 2},   // Add Logical Register
    {"SLR", 2},   // Subtract Logical Register

    // --- RX format (4 bytes) ---------------------------------------
    {"L", 4},    // Load fullword
    {"A", 4},    // Add fullword
    {"LA", 4},   // Load address
    {"ST", 4},   // Store fullword
    {"STC", 4},  // Store Character
    {"STCH", 4}, // Store Halfword
    {"LH", 4},   // Load Halfword
    {"MVC", 4},  // Move Character
    {"MVCL", 4}, // Move Character Long (extended)
    {"MVI", 4},  // Move Immediate
    {"MOV", 4},  // Move Numeric
    {"ZAP", 4},  // Zero and Add Packed
    {"IC", 4},   // Insert Character

    // --- RS format (4 bytes) ---------------------------------------
    {"ED", 4},   // Edit (c,d)
    {"EDMK", 4}, // Edit and Mark
    {"CL", 4},   // Compare Logical Character
    {"CLC", 4},  // Compare Logical
    {"CLI", 4},  // Compare Logical Immediate
    {"CVB", 4},  // Convert to Binary
    {"CVD", 4},  // Convert to Decimal

    // --- SI format (4 bytes) ---------------------------------------
    {"SVC", 4}, // Supervisor Call

    // --- SS format (6 bytes) ---------------------------------------
    {"LM", 6},   // Load Multiple
    {"STM", 6},  // Store Multiple
    {"MVCL", 6}, // Move Character Long (6‑byte variant)
    {"MVZ", 6},  // Move with Offset
    {"EDMK", 6}, // Edit and Mark (6‑byte variant)

    // sentinel
    {"", 0}};

Sym_Entry symtab[MAXSYM];
int symCount = 0;

Lit_Entry littab[MAXLIT];
int litCount = 0;

int mot_length(const char *opc)
{
    for (int i = 0; MOT[i].length; i++)
    {
        if (strcmp(MOT[i].mnemonic, opc) == 0)
            return MOT[i].length;
    }
    return 0;
}

void add_symbol(const char *lbl, int addr)
{
    for (int i = 0; i < symCount; i++)
        if (strcmp(symtab[i].symbol, lbl) == 0)
            return;
    strcpy(symtab[symCount].symbol, lbl);
    symtab[symCount].address = addr;
    symCount++;
}

void add_literal(const char *lit)
{
    for (int i = 0; i < litCount; i++)
        if (strcmp(littab[i].literal, lit) == 0)
            return;
    strcpy(littab[litCount].literal, lit);
    littab[litCount].address = -1;
    litCount++;
}

int main()
{
    FILE *src = fopen("test/source.asm", "r");
    FILE *intf = fopen("intermediate.dat", "w");
    if (!src || !intf)
    {
        perror("File open error");
        return 1;
    }

    char line[128], label[32], opcode[16], operand[64];
    int LC = 0;

    while (fgets(line, sizeof line, src))
    {
        label[0] = opcode[0] = operand[0] = '\0';
        sscanf(line, "%s %s %[^\n]", label, opcode, operand);

        if (strcmp(opcode, "START") == 0)
        {
            LC = atoi(operand);
            fprintf(intf, "%04X %s %s %s\n", LC, label, opcode, operand);
            continue;
        }

        if (strcmp(opcode, "END") == 0)
        {
            for (int i = 0; i < litCount; i++)
            {
                littab[i].address = LC;
                LC += 4;
            }
            fprintf(intf, "%04X %s %s %s\n", LC, label, opcode, operand);
            break;
        }

        int len = mot_length(opcode);
        if (len > 0)
        {
            if (strcmp(label, "**") != 0)
                add_symbol(label, LC);
            if (operand[0] == '=')
                add_literal(operand);
            fprintf(intf, "%04X %s %s %s\n", LC, label, opcode, operand);
            LC += len;
        }
        else if (strcmp(opcode, "DC") == 0 || strcmp(opcode, "DS") == 0)
        {
            if (strcmp(label, "**") != 0)
                add_symbol(label, LC);
            fprintf(intf, "%04X %s %s %s\n", LC, label, opcode, operand);
            LC += 4;
        }
        else
        {
            fprintf(intf, "%04X %s %s %s\n", LC, label, opcode, operand);
        }
    }

    fclose(src);
    fclose(intf);

    printf("SYMBOL TABLE:\n");
    for (int i = 0; i < symCount; i++)
        printf("%-10s : %04X\n", symtab[i].symbol, symtab[i].address);

    printf("\nLITERAL TABLE:\n");
    for (int i = 0; i < litCount; i++)
        printf("%-10s : %04X\n", littab[i].literal, littab[i].address);

    return 0;
}
