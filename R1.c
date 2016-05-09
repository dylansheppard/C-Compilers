// Hand-written S2 compiler in C
#include <stdio.h>  // needed by I/O functions
#include <stdlib.h> // needed by malloc and exit
#include <string.h> // needed by str functions
#include <ctype.h>  // needed by isdigit, etc.
#include <time.h>   // needed by asctime

// Constants

// No boolean type in C.
// Any nonzero number is true.  Zero is false.
#define TRUE 1
#define FALSE 0

// Sizes for arrays
#define MAX 180            // size of string arrays
#define SYMTABSIZE 1000    // symbol table size

#define END 0
#define PRINTLN 1
#define UNSIGNED 2
#define ID 3
#define ASSIGN 4
#define SEMICOLON 5
#define LEFTPAREN 6
#define RIGHTPAREN 7
#define PLUS 8
#define MINUS 9
#define TIMES 10
#define ERROR 11
#define PRINT 12
#define LEFTBRACKET 13
#define RIGHTBRACKET 14
#define DIVIDE 15

time_t timer;    // for asctime

// Prototypes
// Function definition or prototype must
// precede function call so compiler can
// check for correct type, number of args
int expr(void);

void statementList(void);

void emitInt(char *op, int num);

//int factorList(int left);

// Global Variables

// tokenImage used in error messages.  See consume function.
char *tokenImage[16] =
{
    "<END>",
    "\"println\"",
    "<UNSIGNED>",
    "<ID>",
    "\"=\"",
    "\";\"",
    "\"(\"",
    "\")\"",
    "\"+\"",
    "\"-\"",
    "\"*\"",
    "<ERROR>",
     "\"print\"",
     "\"{\"",
      "\"}\"",
    "\"/\""
    
};

char inFileName[MAX], outFileName[MAX], inputLine[MAX];
int debug = FALSE;


char *symbol[SYMTABSIZE];     // symbol table
char *dwValue[SYMTABSIZE];      // new arrays for R1 that parralel SYMBTAB size
int needsDW[SYMTABSIZE];

int symbolx;                  // index into symbol table

//create new type named TOKEN
typedef struct tokentype
{
    int kind;
    int beginLine, beginColumn, endLine, endColumn;
    char *image;
    struct tokentype *next;
} TOKEN;


FILE *inFile, *outFile;     // file pointers

int currentChar = '\n';
int currentColumnNumber;
int currentLineNumber;
TOKEN *currentToken;
TOKEN *previousToken;

//-----------------------------------------
// Abnormal end.
// Close files so S2.a has max info for debugging
void abend(void)
{
    fclose(inFile);
    fclose(outFile);
    exit(1);
}
//-----------------------------------------
void displayErrorLoc(void)
{
    printf("Error on line %d column %d\n", currentToken ->
           beginLine, currentToken -> beginColumn);
}
//-----------------------------------------
// enter symbol into symbol table if not already there
int enter(char *s, char *v, int boo)
{
    int i = 0;
    while (i < symbolx)
    {
        if (!strcmp(s, symbol[i]))
            break;
        i++;
    }
    
    if (i >= 0) {
        return i; //already in symbol table, return it
    }

    else {
    // if s is not in symbol table, then add it
    
    if (i == symbolx)
        if (symbolx < SYMTABSIZE) {
            i = symbolx++;
            symbol[i] = s; //add symbol
            dwValue[i] = v;
            needsDW[i] = boo;
            return i;
        }
        else
        {
            printf("System error: symbol table overflow\n");
            abend();
        }

    }
}
//-----------------------------------------
void getNextChar(void)
{
    if (currentChar == END)
        return;
    
    if (currentChar == '\n')        // need next line?
    {
        // fgets returns 0 (false) on EOF
        if (fgets(inputLine, sizeof(inputLine), inFile))
        {
            // output source line as comment
            fprintf(outFile, "; %s", inputLine);
            currentColumnNumber = 0;
            currentLineNumber++;
        }
        else  // at end of file
        {
            currentChar = END;
            return;
        }
    }
    
    // get next char from inputLine
    currentChar =  inputLine[currentColumnNumber++];
    //S2 NEW FEATURE: detect single line comments -
    
    if(currentChar == '/' && inputLine[currentColumnNumber] == '/') {
        currentChar = '\n'; //assigning char to newLine omits comment, sparing it from being compiled
    }
  
}
//---------------------------------------
// This function is tokenizer (aka lexical analyzer, scanner)
TOKEN *getNextToken(void)
{
    char buffer[80];    // buffer to build image
    int bufferx = 0;    // index into buffer
    TOKEN *t;	        // will point to taken struct
    
    
    // skip whitespace
    while (isspace(currentChar))
        getNextChar();
    
    // construct token to be returned to parser
    t = (TOKEN *)malloc(sizeof(TOKEN)); //allocates a space in memory exact size of current TOKEN
    t -> next = NULL;
    
    // save start-of-token position
    t -> beginLine = currentLineNumber;
    t -> beginColumn = currentColumnNumber;
    
    // check for END
    if (currentChar == END)
    {
        t -> image = "<END>";
        t -> endLine = currentLineNumber;
        t -> endColumn = currentColumnNumber;
        t -> kind = END;
    }
    
    else  // check for unsigned int
        
        if (isdigit(currentChar))
        {
            do
            {
                buffer[bufferx++] = currentChar;
                t -> endLine = currentLineNumber;
                t -> endColumn = currentColumnNumber;
                getNextChar();
            } while (isdigit(currentChar));
            
            buffer[bufferx] = '\0';   // term string with null char
            // save buffer as String in token.image
            // strdup allocates space and copies string to it
            t -> image = strdup(buffer);
            t -> kind = UNSIGNED;
        }
    
        else  // check for identifier
            if (isalpha(currentChar))
            {
                do  // build token image in buffer
                {
                    buffer[bufferx++] = currentChar;
                    t -> endLine = currentLineNumber;
                    t -> endColumn = currentColumnNumber;
                    getNextChar();
                } while (isalnum(currentChar));
                
                buffer[bufferx] = '\0';
                
                // save buffer as String in token.image
                t -> image = strdup(buffer);
                
                // check if keyword
                if (!strcmp(t -> image, "println"))
                    t -> kind = PRINTLN;
                //new S2 Keywords (print)
                
                
                else if (!strcmp(t -> image, "print"))
                    t -> kind = PRINT;
            
                
                else  // not a keyword so kind is ID
                    t -> kind = ID;
            }
    
            else  // process single-character token
            {
                switch(currentChar)
                {
                    case '=':
                        t -> kind = ASSIGN;
                        break;
                    case ';':
                        t -> kind = SEMICOLON;
                        break;
                    case '(':
                        t -> kind = LEFTPAREN;
                        break;
                    case ')':
                        t -> kind = RIGHTPAREN;
                        break;
                        
                    case '}':
                        t -> kind = RIGHTBRACKET;
                        break;
                   
                    case '{':
                        t -> kind = LEFTBRACKET;
                        break;
                    case '+':
                        t -> kind = PLUS;
                        break;
                    case '-':
                       
                        t -> kind = MINUS;
                        break;
                    case '*':
                        t -> kind = TIMES;
                        break;
                        
                    case '/':
                        t -> kind = DIVIDE;
                        
                        break;
                    default:
                        t -> kind = ERROR;
                        break;
                }
                
                // save currentChar as string in image field
                t -> image = (char *)malloc(2);  // get space
                (t -> image)[0] = currentChar;   // move in string
                (t -> image)[1] = '\0';
                
                
                // save end-of-token position
                t -> endLine = currentLineNumber;
                t -> endColumn = currentColumnNumber;
                
                getNextChar();  // read beyond end of token
            }
    // token trace appears as comments in output file
    
    // set debug to true to check tokenizer
    if (debug)
        fprintf(outFile,
                "; kd=%3d bL=%3d bC=%3d eL=%3d eC=%3d     im=%s\n",
                t -> kind, t -> beginLine, t -> beginColumn,
                t -> endLine, t -> endColumn, t -> image);
    
    return t;     // return token to parser
}
//-----------------------------------------
//
// Advance currentToken to next token.
//
void advance(void)
{
    static int firstTime = TRUE;
    
    
    if (firstTime)
    {
        firstTime = FALSE;
        currentToken = getNextToken();
    }
    else
    {
        previousToken = currentToken;
        
        // If next token is on token list, advance to it.
        if (currentToken -> next != NULL)
            currentToken = currentToken -> next;
        
        // Otherwise, get next token from token mgr and
        // put it on the list.
        else
            currentToken = (currentToken -> next) =
            getNextToken();
    }
}
//-----------------------------------------
// If the kind of the current token matches the
// expected kind, then consume advances to the next
// token. Otherwise, it throws an exception.
//
void consume(int expected)
{
    if (currentToken -> kind == expected)
        advance();
    else
    {
        displayErrorLoc();
        printf("Scanning %s, expecting %s\n",
               currentToken -> image, tokenImage[expected]);
        abend();
    }
}
//-----------------------------------------
// This function not used in S1
// getToken(i) returns ith token without advancing
// in token stream.  getToken(0) returns
// previousToken.  getToken(1) returns currentToken.
// getToken(2) returns next token, and so on.
//
TOKEN *getToken(int i)
{
    TOKEN *t;
    int j;
    if (i <= 0)
        return previousToken;
    
    t = currentToken;
    for (j = 1; j < i; j++)  // loop to ith token
    {
        // if next token is on token list, move t to it
        if (t -> next != NULL)
            t = (t -> next);
        
        // Otherwise, get next token from token mgr and
        // put it on the list.
        else
            t = (t -> next) = getNextToken();
    }
    return t;
}

//R1 function: adds now done via symbol table and temp variables
int add(int left, int right) {
    emitInt("ld", left);
    emitInt("add", right);
    int temp = getTemp();
    emitInt("st", temp);
    return temp;
}


int mult(int left, int right) {
    emitInt("ld", left);
    emitInt("mult", right);
    int temp = getTemp();
    emitInt("st", temp);
    return temp;
}

//R1 function: returns a unique label of a temp variable to be used by machine code

int getTemp() {
        static int count = 0; //static so it's contents are saved after function call
       char lbuf[10];
       sprintf(lbuf, "@t%d", count++);  
       strdup(lbuf);
       return enter(lbuf, 0, TRUE);
     
}
//-----------------------------------------
// emit one-operand instruction
void emitInstruction1(char *op)
{
    fprintf(outFile, "          %-4s\n", op);
}
//-----------------------------------------
// emit two-operand instruction
// function overloading not supported by C
void emitInstruction2(char *op, char *opnd)
{
    fprintf(outFile,
            "          %-4s      %s\n", op,opnd);
}
//R1 must support integers in machine instructions
void emitInt(char *op, int num) {
    fprintf(outFile, " %-4s      %d\n", op,num);
}
//-----------------------------------------
void emitdw(char *label, char *value)
{
    char temp[80];
    strcpy(temp, label);
    strcat(temp, ":");
    
    fprintf(outFile,
            "%-9s dw        %s\n", temp, value);
}
//-----------------------------------------
void endCode(void)
{
    int i;
    emitInstruction1("\n          halt\n");
    
    // emit dw for each symbol in the symbol table
    for (i=0; i < symbolx; i++) {
        if(needsDW[i] == TRUE)
             emitdw(symbol[i], dwValue[i]);
    }
}
//-----------------------------------------


void assign(int left, int expVal) {
    emitInt("ld", expVal);
    emitInstruction2("st", symbol[left]);

}

void println(int expVal) {
    emitInstruction2("ld", symbol[expVal]);
    emitInstruction1("dout");
    emitInstruction2("pc", "'\\n'");
    emitInstruction1("aout");
    
}
int factor(void)
{
    TOKEN *t;
    char temp[MAX];
    char temp2[20]; //two temps needed for minus
    int index;
    
    printf("%s ", currentToken -> image);
    switch(currentToken -> kind)
    {
        case UNSIGNED:
            t = currentToken;
            consume(UNSIGNED);
            //sprintf(temp, "@%s", t -> image);
             strcpy(temp,"@");
            strcat(temp, t -> image);
            index = enter(temp, t -> image, TRUE);
            return index;
            break;

        case PLUS:
            consume(PLUS);
            t = currentToken;
            consume(UNSIGNED);

           // sprintf(temp, "@%s", t -> image);
             strcpy(temp,"@");
            strcat(temp, t -> image);
            index = enter(temp, t -> image, TRUE);
            return index;
            break;

        case MINUS:
            consume(MINUS);
            t = currentToken;
            consume(UNSIGNED);
            //sprintf(temp2, "@_%s", t -> image);
            strcpy(temp2,"@_");
            strcat(temp2, t -> image);
            strcpy(temp, "-");
            strcat(temp, t -> image);
           index = enter(temp2, temp, TRUE);
            return index;
            break;

        case ID:
            t = currentToken;
            consume(ID);
            index = enter(t -> image, "0", TRUE);
            return index;
            break;
        case LEFTPAREN:
            consume(LEFTPAREN);
            index = expr();
            consume(RIGHTPAREN);
            return index;
            break;
        default:
            displayErrorLoc();
            printf("Scanning %s, expecting factor\n", currentToken ->
                   image);
            abend();
    }
}
//-----------------------------------------
int factorList(int left) {
int right,temp, termVal;


    if (currentToken -> kind == TIMES) {
        consume(TIMES);
        right = factor();
        temp = mult(left,right);
        termVal = factorList(temp);
        return termVal;
    }

    else {
        return left;
    }

    /**
    switch(currentToken -> kind)
    {
        case TIMES:
            consume(TIMES);
            factor();
            emitInstruction1("mult");
            factorList();
            break;
            
        case DIVIDE:
            consume(DIVIDE);
            factor();
            emitInstruction1("div");
            factorList();
            break;
            
        case PLUS:
        case MINUS: //adding MINUS, telling compiler to simply proceed if a MINUS token appears (lambda)
            
            
        case RIGHTPAREN:
        case SEMICOLON:
            ;
            break;
        default:
            displayErrorLoc();
            printf("Scanning %s, expecting op, \")\", or \";\"\n",
                   currentToken -> image);
            abend();
    }

    */
}
//-----------------------------------------
int term(void)
{
    int left, termVal;
    left = factor();
    termVal = factorList(left);
    return termVal;
}
//-----------------------------------------
int termList(int left)
{
    int right, temp, expVal;

    switch(currentToken -> kind)
    {
        case PLUS:
            consume(PLUS);
            right = term(); //returns next term (since left is current term)
            temp = add(left, right);
            expVal = termList(temp);
            return expVal;
            break;
        case RIGHTPAREN:
        case SEMICOLON:
            return left;
            break;
        default:
            displayErrorLoc();
            printf(
                   "Scanning %s, expecting \"+\", \")\", or \";\"\n",
                   currentToken -> image);
            abend();
    }

    /*

    switch(currentToken -> kind)
    {
        case PLUS:
            consume(PLUS);
            right = term(); //returns next term (since left is current term)
            temp = add(left, right);
            expVal = termList(temp);
            term();
            emitInstruction1("add");
            return expVal;
            break;
            
        case MINUS:
            
            consume(MINUS);
            term();
            emitInstruction1("sub");
            termList();
            break;
            
        case RIGHTPAREN:
        case SEMICOLON:
            ;
            break;
        default:
            displayErrorLoc();
            printf(
                   "Scanning %s, expecting \"+\", \")\", or \";\"\n",
                   currentToken -> image);
            abend();
    }

    */
}

//-----------------------------------------
int expr(void)
{
    int left, expVal;
    left = term();
    expVal = termList(left);
    return expVal;
}
//-----------------------------------------
void assignmentStatement(void)
{
    TOKEN *t;
    int left, expVal;
    t = currentToken;
    consume(ID);
    left = enter(t -> image, "0", TRUE); //enter left value into symbol table


    consume(ASSIGN);
    expVal = expr();
    assign(left, expVal);
  
    consume(SEMICOLON);
}
//-----------------------------------------
void printlnStatement(void)
{
    int expVal;
    consume(PRINTLN);
    consume(LEFTPAREN);
    expVal = expr();
    println(expVal);

    consume(RIGHTPAREN);
    consume(SEMICOLON);
}

void printStatement(void) {
    consume(PRINT);
    consume(LEFTPAREN);
    expr();
    emitInstruction1("dout");

    consume(RIGHTPAREN);
    consume(SEMICOLON);
}
void nullStatement(void) {
    consume(SEMICOLON);
}

void compoundStatement(void) {
    consume(LEFTBRACKET);
    statementList();
    consume(RIGHTBRACKET);
    
    
}
//-----------------------------------------
void statement(void)
{
   
    switch(currentToken -> kind)
    {
        case ID:
            assignmentStatement();
            break;
        case PRINTLN:
            printlnStatement();
            break;
            
 
            
        
            
        default:
            displayErrorLoc();
            printf("Scanning %s, expecting statement\n",
                   currentToken -> image);
            abend();
    }
}
//-----------------------------------------
void statementList(void)
{
    switch(currentToken -> kind)
    {
        case ID:
        case PRINTLN:
            statement();
            statementList();
            break;
            
        case PRINT:
            statement();
            statementList();
            break;
            
        case SEMICOLON:
            statement();
            statementList();
            break;
            
        case LEFTBRACKET:
            statement();
            statementList();
            break;
            
        case RIGHTBRACKET:
        case END:
            ;
            break;
        default:
            displayErrorLoc();
            printf(
                   "Scanning %s, expecting statement or end of file\n",
                   currentToken -> image);
            abend();
    }
}
//-----------------------------------------


void program(void)
{
    statementList();
    endCode();
}
//-----------------------------------------
void parse(void)
{
    advance();
    program();   // program is start symbol for grammar
}
//-----------------------------------------
int main(int argc, char *argv[])
{
    printf("S2 compiler written by Anthony J. Dos Reis\n");
    if (argc != 2)
    {
        printf("Incorrect number of command line args\n");
        exit(1);
    }
    
    
    // build the input and output file names
    strcpy(inFileName, argv[1]);
    strcat(inFileName, ".s");       // append extension
    
    strcpy(outFileName, argv[1]);
    strcat(outFileName, ".a");      // append extension
    
    inFile = fopen(inFileName, "r");
    if (!inFile)
    {
        printf("Error: Cannot open %s\n", inFileName);
        exit(1);
    }
    outFile = fopen(outFileName, "w");
    if (!outFile)
    {
        printf("Error: Cannot open %s\n", outFileName);
        exit(1);
    }
    
    time(&timer);     // get time
    fprintf(outFile, "; Anthony J. Dos Reis    %s",
            asctime(localtime(&timer)));


    fprintf(outFile,
            "; Output from S2 compiler\n");
    fprintf(outFile, "!r"); //using register instruction set 
    
    parse();
    
    fclose(inFile);
    
    // must close output file or will lose most recent writes
    fclose(outFile);
    
    // 0 return code means compile ended without error
    return 0;
}