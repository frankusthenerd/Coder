// ============================================================================
// Coder Assembler (Definitions)
// Programmed by Francois Lamini
// ============================================================================

#include "..\Code_Helper\Codeloader.hpp"
#include "..\Code_Helper\Allegro.hpp"

namespace Codeloader {

  enum eInstruction {
    eINST_COPY,
    eINST_ADD,
    eINST_SUB,
    eINST_MUL,
    eINST_DIV,
    eINST_TEST,
    eINST_JUMP,
    eINST_JSUB,
    eINST_PUSH,
    eINST_POP,
    eINST_RETURN,
    eINST_AND,
    eINST_OR,
    eINST_HALT,
    eINST_INTERRUPT
  };

  enum eAddress {
    eADDRESS_VALUE,
    eADDRESS_IMMEDIATE,
    eADDRESS_POINTER
  };

  enum eTest {
    eTEST_EQUALS,
    eTEST_NOT,
    eTEST_GREATER,
    eTEST_LESS,
    eTEST_GREATER_OR_EQUAL,
    eTEST_LESS_OR_EQUAL
  };

  enum eInterrupt {
    eINTERRUPT_SCREEN,
    eINTERRUPT_INPUT,
    eINTERRUPT_TIMEOUT
  };

  class cASM_Error: public cError {

    public:
      sToken token;

      cASM_Error(sToken token, std::string message);
      void Print();

  };

  class cMemory {

    public:
      int* memory;
      int count;

      cMemory(int size);
      ~cMemory();
      int Read_Number(int address);
      void Write_Number(int address, int value);
      void Clear();

  };
  
  class cSimulator {

    public:
      cMemory* memory;
      int pc;
      int sp;
      int status;
      int interrupt_pointer;
      int width;
      int height;
      int letter_w;
      int letter_h;
      cIO_Control* io;

      cSimulator(cIO_Control* io, std::string config);
      ~cSimulator();
      void Load_Program(std::string name);
      void Save_Program(std::string name);
      void Step();
      void Run(int timeout);
      int Fetch_Number();
      void Put_Number(int number);
      int Fetch_From_Address();
      void Write_To_Address(int value);
      bool Eval_Test();
      void Push(int value);
      int Pop();
      void Process_Interrupt(int interrupt);
      void Draw_Screen(cMemory* memory, int address);

  };

  class cAssembler {

    public:
      cArray<sToken> tokens;
      cHash<std::string, int> symtab;
      cHash<int, std::string> placeholders;
      cSimulator* simulator;
      int pointer;

      cAssembler(cSimulator* simulator);
      void Load_Source(std::string name);
      void Compile_Source(std::string name);
      sToken Parse_Token();
      void Parse_Keyword(std::string keyword);
      void Parse_String();
      void Parse_Address();
      void Parse_Value();
      void Parse_Value(std::string value);
      void Parse_Test();

  };

}
