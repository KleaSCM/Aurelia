/**
 * Aurelia Assembler Command-Line Interface.
 *
 * Provides a UNIX-style command-line interface for assembling Aurelia
 * assembly language programs into binary machine code.
 *
 * PIPELINE STAGES:
 * 1. Lexer: Tokenization (source text → tokens)
 * 2. Parser: Syntax analysis (tokens → AST)
 * 3. Resolver: Symbol resolution (labels → addresses)
 * 4. Encoder: Code generation (AST → binary)
 *
 * USAGE:
 *   asm [options] <input.s>
 *
 * OPTIONS:
 *   -o <file>     Specify output file (default: a.out)
 *   -h, --help    Display help information
 *
 * EXIT CODES:
 *   0  Success (binary generated)
 *   1  Assembly error (lexer/parser/resolver/encoder failure)
 *   2  I/O error (file not found, cannot write)
 *   3  Invalid arguments
 *
 * ERROR HANDLING:
 * - All errors written to stderr with stage identification
 * - Line numbers included for syntax/semantic errors
 * - Status messages to stdout for successful operations
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Encoder.hpp"
#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include "Tools/Assembler/Resolver.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/**
 * Exit code constants following UNIX conventions.
 */
constexpr int ExitSuccess = 0;
constexpr int ExitAssemblyError = 1;
constexpr int ExitIoError = 2;
constexpr int ExitInvalidArgs = 3;

/**
 * @brief Displays usage information to stdout.
 *
 * Shows command syntax, available options, and exit code meanings.
 * Called when user requests help or provides invalid arguments.
 */
void PrintUsage(const char *programName) {
  std::cout << "Aurelia Assembler\n"
            << "Usage: " << programName << " [options] <input.s>\n\n"
            << "Options:\n"
            << "  -o <file>     Specify output binary file (default: a.out)\n"
            << "  -h, --help    Display this help information\n\n"
            << "Exit Codes:\n"
            << "  0  Success\n"
            << "  1  Assembly error\n"
            << "  2  I/O error\n"
            << "  3  Invalid arguments\n\n"
            << "Example:\n"
            << "  " << programName << " -o program.bin program.s\n";
}

/**
 * @brief Reads entire file contents into a string.
 *
 * @param filename Path to file to read.
 * @param[out] content String to store file contents.
 * @return true if file read successfully, false otherwise.
 *
 * IMPLEMENTATION NOTE:
 * Uses std::ifstream with std::istreambuf_iterator for efficient
 * single-allocation read of entire file into memory.
 *
 * ERROR HANDLING:
 * Returns false on failure (file not found, permission denied, etc.).
 * Caller should check return value and report error.
 */
bool ReadFile(const std::string &filename, std::string &content) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  // Read entire file into string buffer
  std::ostringstream buffer;
  buffer << file.rdbuf();
  content = buffer.str();

  file.close();
  return true;
}

/**
 * @brief Writes binary data to file.
 *
 * @param filename Path to output file.
 * @param data Binary data to write.
 * @return true if write successful, false otherwise.
 *
 * ENCODING:
 * Data written in binary mode (no text translation).
 * Output is raw machine code suitable for direct loading into memory.
 *
 * ERROR HANDLING:
 * Returns false if file cannot be created or write fails.
 */
bool WriteFile(const std::string &filename,
               const std::vector<std::uint8_t> &data) {
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  // Write binary data
  file.write(reinterpret_cast<const char *>(data.data()),
             static_cast<std::streamsize>(data.size()));

  if (!file.good()) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

/**
 * @brief Main assembler pipeline orchestration.
 *
 * Coordinates Lexer → Parser → Resolver → Encoder pipeline with
 * error handling at each stage. Provides detailed error reporting
 * identifying which stage failed and why.
 *
 * PIPELINE FLOW:
 *
 *   Source Text
 *       ↓
 *   [Lexer] ────→ Tokens
 *       ↓
 *   [Parser] ───→ AST (ParsedInstructions + Labels + Data)
 *       ↓
 *   [Resolver] ─→ Resolved AST (labels → addresses)
 *       ↓
 *   [Encoder] ──→ Binary Machine Code
 *       ↓
 *   Output File ([text][data])
 *
 * ERROR PROPAGATION:
 * Each stage may fail. On failure, error message is printed to stderr
 * with stage identification and specific error details (line numbers,
 * problem description). Pipeline halts at first failure.
 */
int main(int argc, char *argv[]) {
  /**
   * ARGUMENT PARSING
   *
   * Simple manual parsing without external dependencies.
   * Supports:
   * - Input file (required, positional)
   * - -o <output> (optional, default: a.out)
   * - -h/--help (show usage and exit)
   */
  std::string inputFile;
  std::string outputFile = "a.out";

  // Parse command-line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return ExitSuccess;
    } else if (arg == "-o") {
      // Next argument is output filename
      if (i + 1 >= argc) {
        std::cerr << "Error: -o requires an argument\n";
        PrintUsage(argv[0]);
        return ExitInvalidArgs;
      }
      outputFile = argv[++i];
    } else if (arg[0] == '-') {
      // Unknown option
      std::cerr << "Error: Unknown option: " << arg << "\n";
      PrintUsage(argv[0]);
      return ExitInvalidArgs;
    } else {
      // Positional argument (input file)
      if (!inputFile.empty()) {
        std::cerr << "Error: Multiple input files specified\n";
        PrintUsage(argv[0]);
        return ExitInvalidArgs;
      }
      inputFile = arg;
    }
  }

  // Validate required arguments
  if (inputFile.empty()) {
    std::cerr << "Error: No input file specified\n";
    PrintUsage(argv[0]);
    return ExitInvalidArgs;
  }

  /**
   * STAGE 0: READ INPUT FILE
   */
  std::string sourceCode;
  if (!ReadFile(inputFile, sourceCode)) {
    std::cerr << "Error: Cannot read input file: " << inputFile << "\n";
    return ExitIoError;
  }

  std::cout << "Assembling: " << inputFile << "\n";

  /**
   * STAGE 1: LEXICAL ANALYSIS
   *
   * Tokenize source code into lexical units.
   *
   * INPUT:  Source text (string)
   * OUTPUT: Token stream (vector<Token>)
   *
   * NOTE (KleaSCM) Lexer.Tokenize() returns empty vector if source is invalid.
   * Basic check ensures we got tokens from non-empty source.
   */
  using namespace Aurelia::Tools::Assembler;

  Lexer lexer(sourceCode);
  std::vector<Token> tokens = lexer.Tokenize();

  if (tokens.empty() && !sourceCode.empty()) {
    std::cerr << "Lexer Error: Failed to tokenize source\n";
    return ExitAssemblyError;
  }

  std::cout << "  [✓] Lexer: " << tokens.size() << " tokens\n";

  /**
   * STAGE 2: SYNTAX ANALYSIS
   *
   * Parse token stream into Abstract Syntax Tree.
   *
   * INPUT:  Token stream
   * OUTPUT: ParsedInstructions, Labels, Data segment
   *
   * ERRORS: Syntax errors (unexpected tokens, malformed instructions)
   */
  Parser parser(tokens);
  if (!parser.Parse()) {
    std::cerr << "Parser Error: " << parser.GetErrorMessage() << "\n";
    return ExitAssemblyError;
  }

  // NOTE (KleaSCM) Make mutable copies for Resolver (modifies in-place)
  // and to ensure we're encoding the resolved output.
  auto instructions = parser.GetInstructions();
  auto labels = parser.GetLabels();
  auto dataSegment = parser.GetDataSegment();

  std::cout << "  [✓] Parser: " << instructions.size() << " instructions, "
            << labels.size() << " labels";

  if (!dataSegment.empty()) {
    std::cout << ", " << dataSegment.size() << " data bytes";
  }
  std::cout << "\n";

  // Validate non-empty output
  if (instructions.empty() && dataSegment.empty()) {
    std::cerr << "Warning: Source produces no output (empty program)\n";
  }

  /**
   * STAGE 3: SYMBOL RESOLUTION
   *
   * Two-pass symbol resolution:
   * - Pass 1: Assign addresses to labels
   * - Pass 2: Resolve label references to addresses/offsets
   *
   * INPUT:  ParsedInstructions (with unresolved labels), Labels
   * OUTPUT: ParsedInstructions (labels resolved to immediates)
   *
   * NOTE (KleaSCM) Resolver mutates instruction vector in-place,
   * replacing Label operands with Immediate offsets. We pass our
   * mutable copy and it gets modified during Resolve().
   *
   * ERRORS: Undefined labels, duplicate labels, branch out of range
   */
  Resolver resolver(instructions, labels);
  if (!resolver.Resolve()) {
    std::cerr << "Resolver Error: " << resolver.GetErrorMessage() << "\n";
    return ExitAssemblyError;
  }

  std::cout << "  [✓] Resolver: Symbols resolved\n";

  /**
   * STAGE 4: CODE GENERATION
   *
   * Encode resolved instructions into binary machine code.
   *
   * INPUT:  Resolved ParsedInstructions (modified by Resolver)
   * OUTPUT: Binary byte stream (vector<uint8_t>)
   *
   * NOTE (KleaSCM) Critical: We encode the SAME instruction vector
   * that Resolver modified. Labels have been replaced with immediate
   * offsets by this point. Encoding the original (unresolved) would
   * be a catastrophic bug.
   *
   * ERRORS: Invalid operands, out-of-range immediates, opcode issues
   */
  Encoder encoder(instructions);
  if (!encoder.Encode()) {
    std::cerr << "Encoder Error: " << encoder.GetErrorMessage() << "\n";
    return ExitAssemblyError;
  }

  const auto &binary = encoder.GetBinary();
  std::cout << "  [✓] Encoder: " << binary.size() << " bytes generated\n";

  /**
   * STAGE 5: WRITE OUTPUT FILE
   *
   * OUTPUT FORMAT:
   * Flat binary: [text segment (instructions)][data segment (.string)]
   *
   * NOTE (KleaSCM) Simple flat binary format suitable for direct loading.
   * Text segment (executable code) comes first, data segment (.string
   * directives) follows immediately after. More sophisticated formats
   * (ELF, COFF, etc.) would include headers with section metadata.
   */
  std::vector<std::uint8_t> output;
  output.reserve(binary.size() + dataSegment.size());

  // Append text segment (encoded instructions)
  output.insert(output.end(), binary.begin(), binary.end());

  // Append data segment (.string directives)
  if (!dataSegment.empty()) {
    output.insert(output.end(), dataSegment.begin(), dataSegment.end());
    std::cout << "  [✓] Data: " << dataSegment.size() << " bytes appended\n";
  }

  if (!WriteFile(outputFile, output)) {
    std::cerr << "Error: Cannot write output file: " << outputFile << "\n";
    return ExitIoError;
  }

  std::cout << "Success: Binary written to " << outputFile << " ("
            << output.size() << " bytes total)\n";
  return ExitSuccess;
}
