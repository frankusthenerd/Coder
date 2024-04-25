// ============================================================================
// Coder Assembler (Implementation)
// Programmed by Francois Lamini
// ============================================================================

#include "Coder.h"

Codeloader::cSimulator* simulator = NULL;

bool Source_Process();
bool Process_Keys();

// **************************************************************************
// Program Entry Point
// **************************************************************************

int main(int argc, char** argv) {
  // Initialize Allegro.
  try {
    if (argc == 3) {
      std::string command = argv[1];
      std::string program = argv[2];
      Codeloader::cConfig config("Config");
      int width = config.Get_Property("width");
      int height = config.Get_Property("height");
      if (command == "compile") {
        Codeloader::cAllegro_IO allegro(program, width, height, 2, "Console");
        simulator = new Codeloader::cSimulator(&allegro, "Config");
        Codeloader::cAssembler assembler(simulator);
        assembler.Load_Source(program);
        assembler.Compile_Source(program);
        delete simulator;
      }
      else if (command == "run") {
        Codeloader::cAllegro_IO allegro(program, width, height, 2, "Console");
        simulator = new Codeloader::cSimulator(&allegro, "Config");
        simulator->Load_Program(program);
        allegro.Process_Messages(Source_Process, Process_Keys); // Blocks.
        delete simulator;
      }
      else {
        throw Codeloader::cError("Invalid command " + command + ".");
      }
    }
    else {
      throw Codeloader::cError("Usage: Coder compile | run <program>");
    }
  }
  catch (Codeloader::cASM_Error asm_error) {
    asm_error.Print();
  }
  catch (Codeloader::cError error) {
    error.Print();
  }
  std::cout << "Done." << std::endl;
  return 0;
}

// ****************************************************************************
// Source Processor
// ****************************************************************************

/**
 * Called when command needs to be processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Source_Process() {
  simulator->Run(20);
  return false;
}

/**
 * Called when keys are processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Process_Keys() {
  return false;
}

namespace Codeloader {

  // **************************************************************************
  // Assembly Error Implementation
  // **************************************************************************

  cASM_Error::cASM_Error(sToken token, std::string message) : cError(message) {
    this->token = token;
  }

  /**
   * Prints the assembly error.
   */
  void cASM_Error::Print() {
    std::cout << "*** Error ***" << std::endl;
    std::cout << "Source: " << this->token.source << std::endl;
    std::cout << "Line #: " << this->token.line_no << std::endl;
    std::cout << "Error: " << this->message << std::endl;
  }

  // **************************************************************************
  // Memory Implementation
  // **************************************************************************

  /**
   * Creates a new memory module.
   * @param size The number of units in the memory.
   */
  cMemory::cMemory(int size) {
    this->count = size;
    this->memory = new int[size];
    for (int index = 0; index < size; index++) {
      this->memory[index] = 0;
    }
  }

  /**
   * Frees up the memory.
   */
  cMemory::~cMemory() {
    delete[] this->memory;
  }

  /**
   * Reads a number from the memory given the address.
   * @param address The address of the data to read.
   * @throws An error if the address is invalid.
   */
  int cMemory::Read_Number(int address) {
    int number = 0;
    if ((address >= 0) && (address < this->count)) {
      number = this->memory[address];
      // std::cout << "cell=" << number << ", address=" << address << std::endl;
    }
    else {
      throw cError("Invalid memory access at " + Number_To_Text(address) + ".");
    }
    return number;
  }

  /**
   * Writes a number to the memory at an address.
   * @param address The address to write the number to.
   * @param value The value of the number to write.
   * @throws An error if the memory address is invalid.
   */
  void cMemory::Write_Number(int address, int value) {
    if ((address >= 0) && (address < this->count)) {
      this->memory[address] = value;
    }
    else {
      throw cError("Invalid memory write at " + Number_To_Text(address) + ".");
    }
  }

  /**
   * Clears out the memory.
   */
  void cMemory::Clear() {
    for (int index = 0; index < this->count; index++) {
      this->memory[index] = 0;
    }
  }

  // **************************************************************************
  // Simulator Implementation
  // **************************************************************************

  /**
   * Creates a new simulator.
   * @param io The I/O control reference.
   * @param config The name of the config file.
   * @throws An error if the config file could not be found.
   */
  cSimulator::cSimulator(cIO_Control* io, std::string config) {
    this->io = io;
    this->pc = 0;
    this->sp = 0;
    this->status = eSTATUS_IDLE;
    this->memory = NULL;
    this->interrupt_pointer = 0;
    this->width = 400;
    this->height = 300;
    this->letter_w = 16;
    this->letter_h = 16;
    // Read the configuration file.
    cFile config_file(config + ".txt");
    config_file.Read();
    int memory_size = 200;
    while (config_file.Has_More_Lines()) {
      std::string line = config_file.Get_Line();
      cArray<std::string> pair = Parse_Sausage_Text(line, "=");
      if (pair.Count() == 2) {
        if (pair[0] == "letter-w") {
          this->letter_w = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "letter-h") {
          this->letter_h = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "width") {
          this->width = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "height") {
          this->height = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "interrupt") {
          this->interrupt_pointer = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "memory") {
          memory_size = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "program") {
          this->pc = Text_To_Number(pair[1]);
        }
        else if (pair[0] == "stack") {
          this->sp = Text_To_Number(pair[1]);
        }
        else {
          throw cError("Invalid configuration property " + pair[0] + ".");
        }
      }
      // Anything that is not a pair is a comment.
    }
    // Apply settings.
    this->memory = new cMemory(memory_size);
  }

  /**
   * Frees the simulator.
   */
  cSimulator::~cSimulator() {
    if (this->memory) {
      delete this->memory;
    }
  }

  /**
   * Loads a program to the memory.
   * @param name The name of the program to load.
   * @throws An error if the program could not be loaded.
   */
  void cSimulator::Load_Program(std::string name) {
    cFile prgm_file(name + ".prgm");
    prgm_file.Read();
    int prgm_count = 0;
    while (prgm_file.Has_More_Lines()) {
      int code = 0;
      prgm_file >> code;
      // std::cout << "code=" << code << ", prgm_count=" << prgm_count << std::endl;
      this->memory->Write_Number(prgm_count++, code);
    }
    this->status = eSTATUS_RUNNING;
    std::cout << "Loaded " << prgm_count << " codes into memory." << std::endl;
  }

  /**
   * Saves the program in the memory to a file.
   * @param name The name of the file.
   * @throws An error if the program could not be saved.
   */
  void cSimulator::Save_Program(std::string name) {
    cFile prgm_file(name + ".prgm");
    for (int mem_index = 0; mem_index < this->memory->count; mem_index++) {
      prgm_file.Add(this->memory->Read_Number(mem_index));
    }
    prgm_file.Write();
  }

  /**
   * Steps through a single instruction execution.
   * @throws An error if the instruction is invalid.
   */
  void cSimulator::Step() {
    int instruction = this->memory->Read_Number(this->pc++);
    // std::cout << "instruction=" << instruction << ", pc=" << (this->pc - 1) << std::endl;
    switch (instruction) {
      case eINST_COPY: {
        int value = this->Fetch_From_Address();
        this->Write_To_Address(value);
        break;
      }
      case eINST_ADD: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        this->Write_To_Address(left + right);
        break;
      }
      case eINST_SUB: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        this->Write_To_Address(left - right);
        break;
      }
      case eINST_MUL: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        this->Write_To_Address(left * right);
        break;
      }
      case eINST_DIV: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        if (right == 0) {
          this->Write_To_Address(left); // Do not divide!
        }
        else {
          this->Write_To_Address(left / right);
        }
        break;
      }
      case eINST_TEST: {
        bool result = this->Eval_Test();
        int passed_addr = this->Fetch_Number();
        int failed_addr = this->Fetch_Number();
        // std::cout << "passed=" << passed_addr << ", failed=" << failed_addr << std::endl;
        // if (failed_addr == 1) {
        //   std::cout << "Yes! It is really 1!" << std::endl;
        // }
        if (result) {
          if (passed_addr != TAKE_NO_JUMP) { // Jumps do not need to be taken.
            this->pc = passed_addr;
          }
        }
        else {
          if (failed_addr != TAKE_NO_JUMP) {
            this->pc = failed_addr;
          }
        }
        break;
      }
      case eINST_JUMP: {
        int address = this->Fetch_Number();
        this->pc = address;
        break;
      }
      case eINST_JSUB: {
        int address = this->Fetch_From_Address();
        // std::cout << "jsub=" << address << std::endl;
        this->Push(this->pc); // Push the value to the next instruction after the JSUB.
        this->pc = address;
        break;
      }
      case eINST_PUSH: {
        int value = this->Fetch_From_Address();
        this->Push(value);
        break;
      }
      case eINST_POP: {
        int value = this->Pop();
        this->Write_To_Address(value);
        break;
      }
      case eINST_RETURN: {
        int address = this->Pop(); // Get address to return to.
        this->pc = address;
        break;
      }
      case eINST_AND: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        this->Write_To_Address(left & right);
        break;
      }
      case eINST_OR: {
        int left = this->Fetch_From_Address();
        int right = this->Fetch_From_Address();
        this->Write_To_Address(left | right);
        break;
      }
      case eINST_HALT: {
        this->status = eSTATUS_IDLE;
        break;
      }
      case eINST_INTERRUPT: {
        int interrupt = this->Fetch_Number();
        this->Process_Interrupt(interrupt);
        break;
      }
      default: {
        this->status = eSTATUS_ERROR;
        throw cError("Invalid instruction at " + Number_To_Text(this->pc) + ": " + Number_To_Text(instruction));
      }
    }
  }

  /**
   * Runs the simulator for a certain amount of time before it gives up control.
   * @param timeout The amount of time to run the simulator in milliseconds.
   */
  void cSimulator::Run(int timeout) {
    std::clock_t start = std::clock();
    while (this->status == eSTATUS_RUNNING) {
      std::clock_t end = std::clock();
      std::clock_t diff = (end - start) / CLOCKS_PER_SEC * 1000;
      if ((int)diff < timeout) {
        this->Step();
      }
      else {
        break; // Time is up, break out!
      }
    }
  }

  /**
   * Fetches a number from the memory.
   * @return The fetched number.
   * @throws An error if the number could not be fetched.
   */
  int cSimulator::Fetch_Number() {
    return this->memory->Read_Number(this->pc++);
  }

  /**
   * Puts a number on the memory.
   * @param number The number to put.
   * @throws An error if there is an invalid memory access.
   */
  void cSimulator::Put_Number(int number) {
    this->memory->Write_Number(this->pc++, number);
  }

  /**
   * Fetches a number from an address.
   * @return The fetched number.
   * @throws An error if the number could not be fetched.
   */
  int cSimulator::Fetch_From_Address() {
    int number = 0;
    int addr_mode = this->memory->Read_Number(this->pc++);
    int address = this->memory->Read_Number(this->pc++);
    switch (addr_mode) {
      case eADDRESS_VALUE: {
        number = address; // Address is the value.
        break;
      }
      case eADDRESS_IMMEDIATE: {
        number = this->memory->Read_Number(address);
        break;
      }
      case eADDRESS_POINTER: {
        int pointer = this->memory->Read_Number(address);
        number = this->memory->Read_Number(pointer);
        break;
      }
      default: {
        this->status = eSTATUS_ERROR;
        throw cError("Invalid address mode " + Number_To_Text(addr_mode) + " for read.");
      }
    }
    return number;
  }

  /**
   * Writes a value to the memory at the given address.
   * @param value The value to write to memory.
   * @throws An error if the address mode is invalid. You cannot have a value address mode.
   */
  void cSimulator::Write_To_Address(int value) {
    int addr_mode = this->memory->Read_Number(this->pc++);
    int address = this->memory->Read_Number(this->pc++);
    switch (addr_mode) {
      case eADDRESS_IMMEDIATE: {
        this->memory->Write_Number(address, value);
        break;
      }
      case eADDRESS_POINTER: {
        int pointer = this->memory->Read_Number(address);
        this->memory->Write_Number(pointer, value);
        break;
      }
      default: {
        this->status = eSTATUS_ERROR;
        throw cError("Invalid address mode " + Number_To_Text(addr_mode) + " for write.");
      }
    }
  }

  /**
   * Evaluates a test for the command.
   * @return True if the test passed, false if it failed.
   * @throws An error if the test is invalid.
   */
  bool cSimulator::Eval_Test() {
    int left = this->Fetch_From_Address();
    int test = this->Fetch_Number();
    int right = this->Fetch_From_Address();
    bool result = false;
    int diff = right - left;
    switch (test) {
      case eTEST_EQUALS: {
        result = (diff == 0);
        break;
      }
      case eTEST_NOT: {
        result = (diff != 0);
        break;
      }
      case eTEST_GREATER: {
        result = (diff > 0);
        break;
      }
      case eTEST_LESS: {
        result = (diff < 0);
        break;
      }
      case eTEST_GREATER_OR_EQUAL: {
        result = (diff >= 0);
        break;
      }
      case eTEST_LESS_OR_EQUAL: {
        result = (diff <= 0);
        break;
      }
      default: {
        this->status = eSTATUS_ERROR;
        throw cError("Invalid test " + Number_To_Text(test) + ".");
      }
    }
    return result;
  }

  /**
   * Pushes a value onto the stack.
   * @param value The value to push onto the stack.
   * @throws An error if an invalid memory location is accessed.
   */
  void cSimulator::Push(int value) {
    this->memory->Write_Number(this->sp++, value);
  }

  /**
   * Pops a value off of the stack.
   * @return The value from the stack.
   * @throws An error if an invalid memory location is accessed.
   */
  int cSimulator::Pop() {
    return this->memory->Read_Number(--this->sp);
  }

  /**
   * Processes an interrupt.
   * @param interrupt The interrupt number.
   * @throws An error if the interrupt is not recognized.
   */
  void cSimulator::Process_Interrupt(int interrupt) {
    int pointer = this->memory->Read_Number(this->interrupt_pointer + interrupt);
    switch (interrupt) {
      case eINTERRUPT_INPUT: {
        sSignal key = this->io->Read_Key();
        this->memory->Write_Number(pointer, key.code); // Write out key.
        break;
      }
      case eINTERRUPT_SCREEN: {
        this->Draw_Screen(this->memory, pointer);
        break;
      }
      case eINTERRUPT_TIMEOUT: {
        int delay = this->memory->Read_Number(pointer);
        this->io->Timeout(delay);
        break;
      }
      default: {
        throw cError("Interrupt " + Number_To_Text(interrupt) + " is not defined.");
      }
    }
  }

  /**
   * Draws the character screen to the display.
   * @param memory The memory where the screen is at.
   * @param address The address of the screen.
   */
  void cSimulator::Draw_Screen(Codeloader::cMemory* memory, int address) {
    int grid_w = this->width / this->letter_w;
    int grid_h = this->height / this->letter_h;
    this->io->Color(255, 255, 255); // Color screen to white.
    for (int y = 0; y < grid_h; y++) {
      for (int x = 0; x < grid_w; x++) {
        int letter = memory->Read_Number(address + (y * grid_w) + x);
        char buffer[2] = { (char)letter, 0 };
        this->io->Output_Text(buffer, x * this->letter_w, y * this->letter_h, 0, 0, 255);
      }
    }
    // Draw screen to display.
    this->io->Refresh();
  }

  // **************************************************************************
  // Assembler Implementation
  // **************************************************************************

  /**
   * Creates a new assembler.
   * @param simulator The simulator to populate with code.
   */
  cAssembler::cAssembler(cSimulator* simulator) {
    this->simulator = simulator;
    this->pointer = 0;
  }

  /**
   * Loads source code and generate tokens.
   * @param The name of the source code file.
   * @throws An error if the source could not be loaded.
   */
  void cAssembler::Load_Source(std::string name) {
    std::ifstream source_file(name + ".asm");
    if (source_file) {
      int line_no = 0;
      while (!source_file.eof()) {
        std::string line;
        std::getline(source_file, line);
        if (source_file.good()) {
          line_no++;
          if (line.length() > 0) {
            if (line[0] == ':') { // Code line.
              std::string code_line = line.substr(1);
              cArray<std::string> tokens = Parse_Sausage_Text(code_line, " ");
              int token_count = tokens.Count();
              for (int token_index = 0; token_index < token_count; token_index++) {
                sToken token;
                token.token = tokens[token_index];
                token.line_no = line_no;
                token.source = name;
                this->tokens.Push(token);
                // std::cout << "token=" << token.token << std::endl;
              }
            }
          }
        }
      }
    }
    else {
      throw cError("Could not load " + name + ".");
    }
  }

  /**
   * Compiles the source code.
   * @param name The name of the source file.
   * @throws An error if there is a syntax error.
   */
  void cAssembler::Compile_Source(std::string name) {
    // Add default definitions.
    // Add interrupts.
    this->symtab["{screen}"] = eINTERRUPT_SCREEN;
    this->symtab["{input}"] = eINTERRUPT_INPUT;
    this->symtab["{timeout}"] = eINTERRUPT_TIMEOUT;
    // Add meta information.
    this->symtab["{memory}"] = this->simulator->memory->count;
    this->symtab["{width}"] = this->simulator->width;
    this->symtab["{height}"] = this->simulator->height;
    this->symtab["{letter-w}"] = this->simulator->letter_w;
    this->symtab["{letter-h}"] = this->simulator->letter_h;
    this->symtab["{grid-w}"] = this->simulator->width / this->simulator->letter_w;
    this->symtab["{grid-h}"] = this->simulator->height / this->simulator->letter_h;
    this->symtab["{take-no-jump}"] = TAKE_NO_JUMP;
    // Add all readable characters.
    for (char letter = '!'; letter <= '~'; letter++) {
      char letter_str[2] = { letter, 0 };
      this->symtab["(" + std::string(letter_str) + ")"] = (int)letter;
    }
    // Add non-readable or unparsable characters.
    this->symtab["(space)"] = (int)' ';
    this->symtab["(backspace)"] = eSIGNAL_BACKSPACE;
    this->symtab["(delete)"] = eSIGNAL_DELETE;
    this->symtab["(enter)"] = eSIGNAL_ENTER;
    this->symtab["(tab)"] = eSIGNAL_TAB;
    // Parse instructions.
    while (this->tokens.Count() > 0) {
      sToken instruction = this->Parse_Token();
      if (instruction.token == "define") {
        sToken name = this->Parse_Token();
        this->Parse_Keyword("as");
        sToken number = this->Parse_Token();
        this->symtab["[" + name.token + "]"] = Text_To_Number(number.token);
      }
      else if (instruction.token == "number") {
        sToken number = this->Parse_Token();
        this->simulator->memory->Write_Number(this->pointer++, Text_To_Number(number.token));
      }
      else if (instruction.token == "list") {
        sToken count = this->Parse_Token();
        this->pointer += Text_To_Number(count.token);
      }
      else if (instruction.token == "objects") {
        sToken dimension = this->Parse_Token();
        cArray<std::string> pair = Parse_Sausage_Text(dimension.token, "x");
        if (pair.Count() == 3) {
          int object_size = Text_To_Number(pair[0]);
          int property_size = Text_To_Number(pair[1]);
          int object_count = Text_To_Number(pair[2]);
          this->pointer += (object_size * property_size * object_count);
        }
        else {
          throw cASM_Error(dimension, "Invalid format for dimension.");
        }
      }
      else if (instruction.token == "label") {
        sToken name = this->Parse_Token();
        this->symtab["[" + name.token + "]"] = this->pointer;
      }
      else if (instruction.token == "string") {
        this->Parse_String();
      }
      else if (instruction.token == "object") {
        sToken name = this->Parse_Token();
        sToken property = { "", 0, "" };
        int prop_index = 0;
        while (property.token != "end") {
          property = this->Parse_Token();
          this->symtab["[" + name.token + "->" + property.token + "]"] = prop_index++;
        }
      }
      else if (instruction.token == "map") {
        sToken element = { "", 0, "" };
        int element_index = 0;
        while (element.token != "end") {
          element = this->Parse_Token();
          this->symtab["[" + element.token + "]"] = element_index++;
        }
      }
      else if (instruction.token == "copy") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_COPY);
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "add") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_ADD);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "sub") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_SUB);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "mul") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_MUL);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "div") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_DIV);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "test") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_TEST);
        this->Parse_Address(); // Condition
        this->Parse_Test();
        this->Parse_Address();
        this->Parse_Value(); // Jump if passed.
        this->Parse_Value(); // Jump if failed.
      }
      else if (instruction.token == "jump") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_JUMP);
        this->Parse_Value();
      }
      else if (instruction.token == "jsub") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_JSUB);
        this->Parse_Address();
      }
      else if (instruction.token == "push") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_PUSH);
        this->Parse_Address();
      }
      else if (instruction.token == "pop") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_POP);
        this->Parse_Address();
      }
      else if (instruction.token == "return") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_RETURN);
      }
      else if (instruction.token == "and") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_AND);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "or") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_OR);
        this->Parse_Address();
        this->Parse_Address();
        this->Parse_Address();
      }
      else if (instruction.token == "halt") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_HALT);
      }
      else if (instruction.token == "interrupt") {
        this->simulator->memory->Write_Number(this->pointer++, eINST_INTERRUPT);
        this->Parse_Value();
      }
      else {
        throw cASM_Error(instruction, "Invalid instruction.");
      }
    }
    // Resolve placeholders.
    int placeholder_count = this->placeholders.Count();
    for (int placeholder_index = 0; placeholder_index < placeholder_count; placeholder_index++) {
      int location = this->placeholders.keys[placeholder_index];
      std::string name = this->placeholders.values[placeholder_index];
      // Look for placeholder in symbol table.
      if (this->symtab.Does_Key_Exist(name)) { // Found!
        int value = this->symtab[name];
        this->simulator->memory->Write_Number(location, value); // Write value to memory location.
      }
      else {
        throw cError("Could not find placeholder " + name + ".");
      }
    }
    // Save the program to disk.
    this->simulator->Save_Program(name);
  }

  /**
   * Parses a token from the token stack.
   * @returns The token object.
   * @throws An error if there are no more tokens.
   */
  sToken cAssembler::Parse_Token() {
    sToken token = { "", 0, "" };
    if (this->tokens.Count() > 0) {
      token = this->tokens.Shift();
    }
    else {
      throw cError("Out of tokens.");
    }
    return token;
  }

  /**
   * Parses a keyword.
   * @param The keyword.
   * @throws An error if the keyword is missing.
   */
  void cAssembler::Parse_Keyword(std::string keyword) {
    sToken token = this->Parse_Token();
    if (token.token != keyword) {
      throw cASM_Error(token, "Keyword " + keyword + " is missing.");
    }
  }

  /**
   * Parses a string and stores it in C-Lesh format.
   */
  void cAssembler::Parse_String() {
    sToken word = this->Parse_Token();
    std::string text = "";
    if (word.token[0] == '"') {
      if (word.token[word.token.length() - 1] == '"') {
        text += word.token.substr(1, word.token.length() - 2);
      }
      else {
        text += word.token.substr(1);
        text += ' '; // Add the space.
        while (word.token[word.token.length() - 1] != '"') { // Did we hit end of string?
          word = this->Parse_Token();
          if (word.token[word.token.length() - 1] == '"') {
            text += word.token.substr(0, word.token.length() - 1);
          }
          else {
            text += word.token;
            text += ' ';
          }
        }
      }
    }
    else {
      throw cASM_Error(word, "Not the start of a string.");
    }
    // Write out the string.
    if (text.length() > 0) {
      int letter_count = text.length();
      this->simulator->memory->Write_Number(this->pointer++, letter_count); // Write string size first.
      for (int letter_index = 0; letter_index < letter_count; letter_index++) {
        this->simulator->memory->Write_Number(this->pointer++, (int)text[letter_index]);
      }
    }
  }

  /**
   * Parses an address.
   * @throws An error if the address is invalid.
   */
  void cAssembler::Parse_Address() {
    sToken address = this->Parse_Token();
    std::string value = address.token.substr(1);
    if (address.token[0] == '$') { // Immediate value.
      this->simulator->memory->Write_Number(this->pointer++, eADDRESS_VALUE);
      this->Parse_Value(value);
    }
    else if (address.token[0] == '#') { // Immediate address.
      this->simulator->memory->Write_Number(this->pointer++, eADDRESS_IMMEDIATE);
      this->Parse_Value(value);
    }
    else if (address.token[0] == '@') { // Pointer
      this->simulator->memory->Write_Number(this->pointer++, eADDRESS_POINTER);
      this->Parse_Value(value);
    }
    else {
      throw cASM_Error(address, "Invalid addressing mode.");
    }
  }

  /**
   * Parses a value from a token.
   * @throws An error if the value is invalid.
   */
  void cAssembler::Parse_Value() {
    sToken value = this->Parse_Token();
    this->Parse_Value(value.token);
  }

  /**
   * Parses a value.
   * @param value The value to parse. Pass in an empty string to parse a token.
   * @throw An error if the value is invalid.
   */
  void cAssembler::Parse_Value(std::string value) {
    try {
      int number = Text_To_Number(value);
      this->simulator->memory->Write_Number(this->pointer++, number);
    }
    catch (cError number_error) {
      if (value.length() > 0) {
        this->placeholders[this->pointer++] = value; // Mark placeholder.
      }
      else {
        throw cError("Empty placeholder.");
      }
    }
  }

  /**
   * Parses a test.
   * @throws An error if the test is invalid.
   */
  void cAssembler::Parse_Test() {
    sToken test = this->Parse_Token();
    if (test.token == "=") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_EQUALS);
    }
    else if (test.token == "not") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_NOT);
    }
    else if (test.token == ">") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_GREATER);
    }
    else if (test.token == "<") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_LESS);
    }
    else if (test.token == ">or=") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_GREATER_OR_EQUAL);
    }
    else if (test.token == "<or=") {
      this->simulator->memory->Write_Number(this->pointer++, eTEST_LESS_OR_EQUAL);
    }
    else {
      throw cASM_Error(test, "Invalid test.");
    }
  }

}