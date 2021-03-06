/**
 * @file montador.cpp
 * @author Eduardo F. Assis
 * @brief 
 * @version 1.0
 * @date 2020-10-19
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "montador.h"

#include <string.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "tabela.h"

#define LINE_LENGTH 256
#define DELIMETERS_PRE " \t"  // not validate params ("," stays)
#define DELIMETERS_COMP " \t,"

using namespace std;

bool is_number(string &s) {
    bool is_number = true;
    for (size_t i = 0; i < s.length(); i++) {
        if (!isdigit(s[i])) {
            is_number = false;
            break;
        }
    }

    return is_number;
}

map<int, string> _swap_TS(map<string, int> TS) {
    map<int, string> res;
    
    // each key should have only one value
    for (auto &e : TS) {
        res[e.second] = e.first;
    }

    return res;
}

void _obj_one_line(ostream &out_file, vector<Instruction> &text_table, vector<Directive> &data_table) {
    for (Instruction &instr : text_table) {
        out_file << instr.opcode << ' ';

        for (auto &e : instr.operands) {
            out_file << e.value + e.offset << ' ';
        }
    }

    for (Directive &direc : data_table) {
        for (auto &e : direc.operands) {
            out_file << e.value << ' ';
        }
    }
}

void _obj_pretty(ostream &out_file, vector<Instruction> &text_table, vector<Directive> &data_table) {
    int end_counter = 0;

    for (Instruction &instr : text_table) {
        string end = "End " + to_string(end_counter) + ":";
        out_file << setw(10) << left << end << setw(6) << right << instr.opcode;

        for (auto &e : instr.operands) {
            out_file << setw(6) << right << e.value + e.offset;
        }
        out_file << endl;
        end_counter += instr.length;
    }

    for (Directive &direc : data_table) {
        for (auto &e : direc.operands) {
            string end = "End " + to_string(end_counter) + ":";
            out_file << setw(10) << left << end << setw(6) << right << e.value << endl;

            end_counter += direc.length;
        }
    }
}

void _obj_x86(ostream &out_file, AssemblyTables tables) {
    map<int, string> label_at_address = _swap_TS(tables.TS);

    // out_file << "extern LerInteiro, EscreverInteiro\n";
    // out_file << "extern LerChar, EscreverChar\n";
    // out_file << "extern LerString, EscreverString\n";
    // out_file << "extern Overflow\n\n";
    out_file << "%include \"inout.asm\"\n\n";

    // Data ----------------------------------------------
    bool has_data = false;
    bool has_bss = false;
    for (Directive &direc : tables.data_table) {
        if (direc.name == "CONST") {
            if (!has_data) {
                has_data = true;

                out_file << "section .data\n";
            }

            string s_out = TranslationTable.at(direc.name);     // output string
            
            for (auto &e : direc.operands) {
                size_t found = s_out.find('*');
                s_out.replace(found, 1, to_string(e.value));
            }
            s_out = "\t" + direc.label + " " + s_out;

            out_file << s_out;
        }
    }
    out_file << '\n';

    for (Directive &direc : tables.data_table) {
        if (direc.name == "SPACE") {
            if (!has_bss) {
                has_bss = true;

                out_file << "section .bss\n";
            }

            string s_out = TranslationTable.at(direc.name);     // output string
            
            size_t found = s_out.find('*');
            s_out.replace(found, 1, to_string(direc.operands.size()));

            s_out = "\t" + direc.label + " " + s_out;

            out_file << s_out;
        }
    }
    out_file << '\n';
    
    // Text ----------------------------------------------
    out_file << "section .text\n";
    out_file << "\tglobal _start\n\n";
    out_file << "_start:\n";
    
    
    for (Instruction &instr : tables.text_table) {
        // arithmetic
        if (instr.name == "ADD" || instr.name == "SUB" || instr.name == "MULT"
                || instr.name == "DIV") {
            string s_out = TranslationTable.at(instr.name);     // output string
            
            for (auto &e : instr.operands) {
                size_t found = s_out.find('*');
                if (e.offset) s_out.replace(found, 1, "[" + label_at_address[e.value] + "+" + to_string(e.offset*4) + "]");
                else s_out.replace(found, 1, "[" + label_at_address[e.value] + "]");
            }
            if (instr.label.size()) out_file << instr.label + ":\n";

            out_file << s_out;
        }
        // jumps
        else if (instr.name == "JMP" || instr.name == "JMPN" || instr.name == "JMPP"
                || instr.name == "JMPZ") {
            string s_out = TranslationTable.at(instr.name);     // output string
            
            for (auto &e : instr.operands) {
                size_t found = s_out.find('*');
                s_out.replace(found, 1, label_at_address[e.value]);
            }
            if (instr.label.size()) out_file << instr.label + ":\n";

            out_file << s_out;
        }
        // memory
        else if (instr.name == "COPY" || instr.name == "LOAD" || instr.name == "STORE") {
            string s_out = TranslationTable.at(instr.name);     // output string
            
            for (auto &e : instr.operands) {
                size_t found = s_out.find('*');
                if (e.offset) s_out.replace(found, 1, "[" + label_at_address[e.value] + "+" + to_string(e.offset*4) + "]");
                else s_out.replace(found, 1, "[" + label_at_address[e.value] + "]");
            }
            if (instr.label.size()) out_file << instr.label + ":\n";

            out_file << s_out;
        }
        // i/o
        else if (instr.name == "INPUT" || instr.name == "OUTPUT"
                || instr.name == "C_INPUT" || instr.name == "C_OUTPUT" 
                || instr.name == "S_INPUT" || instr.name == "S_OUTPUT") {
            string s_out = TranslationTable.at(instr.name);     // output string
            
            for (auto &e : instr.operands) {
                size_t found = s_out.find('*');
                
                if (e.type == ParamType::EndMem) {
                    if (e.offset) s_out.replace(found, 1, label_at_address[e.value] + "+" + to_string(e.offset*4));
                    else s_out.replace(found, 1, label_at_address[e.value]);
                } else if (e.type == ParamType::Immediate) {
                    s_out.replace(found, 1, to_string(e.value));
                }
            }
            if (instr.label.size()) out_file << instr.label + ":\n";

            out_file << s_out;
        }
        // stop
        else if (instr.name == "STOP") {
            string s_out = TranslationTable.at(instr.name);     // output string
            if (instr.label.size()) out_file << instr.label + ":\n";
            out_file << s_out;
        }
        else {
            cout << "ERRO! Instrução não pode ser traduzida." << endl;
        }
    }
}

bool _get_instr(ifstream &in_file, RawInstruction &raw_instr, int &line) {
    // RawInstruction instr;
    bool complete_instr = false;
    bool is_comment = false;
    string label;
    string command;
    int command_line = 0;
    vector<string> operands;
    InstType instr_type;

    vector<string> tokens;
    string token;
    while (!complete_instr && !in_file.eof() && !in_file.bad()) {
        char c;
        in_file.get(c);

        if (c == ';') {  // skipping commments
            is_comment = true;
        }

        if (!is_comment) {
            if (c != '\n' && c != '\r' && c != '\t' && c != ' ' && !in_file.eof()) {  // building token
                token.push_back(toupper(c));
            } else {                                         // formed token
                for (size_t i = 0; i < token.size(); i++) {  // checking token
                    if (!isalnum(token[i]) && token[i] != '_' && token[i] != ',' && token[i] != ':' && token[i] != '+') {
                        cout << "Erro LEXICO! Token invalido \"" << token << "\" "
                             << "(linha " << line << ")." << endl;

                        // label.clear();  // invalidate all previous informations
                        // command.clear();
                        // operands.clear();

                        token.clear();
                        break;
                    }
                }

                // valid token
                if (!token.empty()) {
                    if (token.find(":") != string::npos) {
                        label = token;
                        label.pop_back();
                        command.clear();
                        operands.clear();
                    }

                    // Only checks if a given command exists
                    else if (TI.count(token) > 0) {
                        command = token;
                        command_line = line;
                        instr_type = InstType::Type1;
                        operands.clear();
                    } else if (TD.count(token) > 0) {
                        command = token;
                        command_line = line;
                        instr_type = InstType::Type2;
                        operands.clear();
                    } else if (!command.empty()) {  // pode ser parametros
                        operands.push_back(token);
                    } else {
                        cout << "Erro SINTATICO! Diretiva/Instrucao invalida \"" << token << "\" "
                             << "(linha " << line << ")." << endl;
                    }

                    token.clear();
                }
            }
        }

        // new line
        if (c == '\n') {
            is_comment = false;
            line++;
        }

        // Constructing instr
        if (!command.empty() && (c == '\n' || in_file.eof())) {
            complete_instr = true;

            raw_instr.type = instr_type;
            raw_instr.label = label;
            raw_instr.instr_name = command;
            raw_instr.operands = operands;
            raw_instr.line = command_line;
        }
    }

    return complete_instr;
}

void pre_process(ifstream &in_file, ofstream &out_file) {
    RawInstruction raw_instr;
    int line_count = 1;

    map<string, int> equ_table;

    enum class MajorState { Equ,
                            Text,
                            Data,
                            PassThrough };
    MajorState currentMajorState = MajorState::Equ;
    // MajorState previousMajorState = currentMajorState;

    enum class TextState { Get,
                           Pass };
    TextState currentTextState = TextState::Get;

    while (!in_file.eof() && !in_file.bad()) {
        raw_instr.clear();
        if (_get_instr(in_file, raw_instr, line_count)) {
            // check state
            if (raw_instr.instr_name == "SECTION") {
                if (raw_instr.operands.front() == "TEXT") {
                    currentMajorState = MajorState::Text;
                } else if (raw_instr.operands.front() == "DATA") {
                    currentMajorState = MajorState::Data;
                    // add \n on state transition
                    out_file << endl;
                }
            }

            if (currentMajorState == MajorState::Equ) {
                if (raw_instr.instr_name == "EQU") {
                    if (equ_table.count(raw_instr.label) == 0) {
                        if (is_number(raw_instr.operands.front())) {
                            equ_table[raw_instr.label] = stoi(raw_instr.operands.front());
                        } else {
                            cout << "Erro SINTATICO! Tipo de operando invalido \"" << raw_instr.operands.front() << "\" "
                                 << "(linha " << raw_instr.line << ")." << endl;
                        }
                    } else {
                        cout << "Erro SEMANTICO! Rotulo repetido \"" << raw_instr.label << "\" "
                             << "(linha " << raw_instr.line << ")." << endl;
                    }
                }
            } else if (currentMajorState == MajorState::Text || currentMajorState == MajorState::Data) {
                if (raw_instr.instr_name == "IF") {
                    if (equ_table.count(raw_instr.operands.front()) > 0) {  // check param(label) on equ_table
                        if (equ_table.at(raw_instr.operands.front()) != 1) {
                            currentTextState = TextState::Pass;
                        }
                    } else {  // didnt find correspondent label
                        currentTextState = TextState::Pass;
                        cout << "Erro SEMANTICO! Rotulo ausente \"" << raw_instr.operands.front() << "\" "
                             << "(linha " << raw_instr.line << ")." << endl;
                    }
                } else if (currentTextState == TextState::Get) {
                    if (!raw_instr.label.empty()) {
                        out_file << raw_instr.label << ": " << raw_instr.instr_name << ' ';
                    } else {
                        out_file << raw_instr.instr_name << ' ';
                    }
                    for (auto &e : raw_instr.operands) {
                        if (!is_number(e) && equ_table.count(e) > 0) {
                            out_file << equ_table.at(e) << ' ';
                        } else {
                            out_file << e << ' ';
                        }
                    }
                    out_file << '\n';
                } else if (currentTextState == TextState::Pass) {
                    currentTextState = TextState::Get;
                }
            }
        }
    }
}

AssemblyTables generate_tables(ifstream &in_file) {
    enum class MajorState { FirstPass,
                            SecondPass,
                            PassThrough };
    MajorState currentMajorState = MajorState::FirstPass;
    MajorState previousMajorState = currentMajorState;

    enum class Section { None,
                         Text,
                         Data };
    Section currentSection = Section::None;

    bool stop = false;
    bool fatal_error = false;
    int line_count = 0;
    int end_count = 0;

    bool found_text = false;

    // tabela final
    AssemblyTables assembly_table;

    // Tabela de Símbolos
    map<string, int> TS;

    // each: end. mem, opcode, param1 (end. mem), ...
    vector<Instruction> text_table;

    // end. mem / content
    vector<Directive> data_table;
    // map<int, int> data_table;

    // bool found_data = false;

    while (!stop) {  // multiple passes into the file
        while (!in_file.eof() && !in_file.bad()) {
            char line[LINE_LENGTH];

            // Line
            in_file.getline(line, LINE_LENGTH);
            line_count++;

            // Tokenize
            vector<string> token;
            char *pch;
            if (strlen(line) > 0) {
                pch = strtok(line, DELIMETERS_COMP);
                while (pch != NULL) {
                    token.push_back(pch);

                    pch = strtok(NULL, DELIMETERS_COMP);
                }
            }

            // specifing
            string label;
            string command;
            vector<string> param;

            for (string &e : token) {
                if (e.find(":") != string::npos) {  // label
                    if (label.empty()) {
                        if (e.size() < 50 && isalpha(e[0])) {
                            label = e;
                            label.pop_back();
                        } else if (currentMajorState == MajorState::SecondPass) {
                            cout << "Erro LEXICO! Rotulo invalido \"" << e << "\" "
                                 << "(linha " << line_count << ")." << endl;
                        }
                    } else if (currentMajorState == MajorState::SecondPass) {  // show errors one time only
                        cout << "Erro SINTATICO! Mais de um rotulo na mesma linha "
                             << "(linha " << line_count << ")." << endl;
                    }
                } else if (TI.count(e) > 0) {  // instruction
                    if (command.empty()) {
                        command = e;
                    } else if (currentMajorState == MajorState::SecondPass) {
                        cout << "Erro SINTATICO! Instrucao com tipo de operando invalido \"" << e << "\" "
                             << "(linha " << line_count << ")." << endl;
                    }
                } else if (TD.count(e) > 0) {  // directive
                    if (command.empty()) {
                        command = e;
                    } else if (currentMajorState == MajorState::SecondPass) {
                        cout << "Erro SINTATICO! Diretiva com tipo de operando invalido \"" << e << "\" "
                             << "(linha " << line_count << ")." << endl;
                    }
                } else {  // parameters isn't checked yet
                    param.push_back(e);
                }
            }

            // Empty line (\n)
            if (token.empty()) {
                currentMajorState = MajorState::PassThrough;
            }

            // Check section
            if (command == "SECTION") {
                if (param.size() == TD.at("SECTION").qtd_operands) {
                    if (param.front() == "TEXT") {
                        found_text = true;
                        currentSection = Section::Text;
                    } else if (param.front() == "DATA") {
                        currentSection = Section::Data;
                    } else if (currentMajorState == MajorState::SecondPass) {
                        cout << "Erro SINTATICO! Secao invalida \"" << param.front() << "\" "
                             << "(linha " << line_count << ")." << endl;
                    }
                } else if (currentMajorState == MajorState::SecondPass) {  // show error only on second pass
                    cout << "Erro SINTATICO! Diretiva  \"" << command << "\" com quantidade de operandos invalida "
                         << "(linha " << line_count << ")." << endl;
                }

                currentMajorState = MajorState::PassThrough;
            }

            if (currentMajorState == MajorState::FirstPass) {  // make TS, check labels and directives
                bool is_valid = false;       // command is valid?            
                
                if (currentSection == Section::Text) {
                    if (!label.empty()) {
                        if (TS.count(label) == 0) {
                            TS[label] = end_count;
                            is_valid = true;
                        } else {
                            cout << "Erro SEMANTICO! Rotulo repetido \"" << label << "\" "
                                 << "(linha " << line_count << ")." << endl;
                        }
                    } else {
                        is_valid = true;
                    }
                } else if (currentSection == Section::Data) {
                    if (!label.empty()) {
                        if (TS.count(label) == 0) {
                            Directive direc;
                            // bool is_valid = true;

                            if (TD.count(command)) {
                                direc = TD.at(command);
                                direc.label = label;
                                
                                TS[label] = end_count;

                                if (param.size()) {
                                    if (param.size() == TD.at(command).qtd_operands) {
                                        if (is_number(param.front())) {
                                            int aux = stoi(param.front());
                                            // does nothing for others directives
                                            if (command == "SPACE") {
                                                for (int i = 0; i < aux; i++) {
                                                    Param defined_param;
                                                    defined_param.type = ParamType::Immediate; defined_param.value = 0;
                                                    
                                                    direc.operands.emplace_back(defined_param);
                                                }

                                                end_count += aux - 1;    // -1 because we will increment at the end of loop
                                            
                                            } else if (command == "CONST") {
                                                Param defined_param;
                                                defined_param.type = ParamType::Immediate; defined_param.value = aux;
                                                
                                                direc.operands.emplace_back(defined_param);
                                            }
                                            is_valid = true;

                                        } else {
                                            cout << "Erro SINTATICO! Diretiva \"" << command << "\"  com o tipo de operandos invalido "
                                                 << "(linha " << line_count << ")." << endl;
                                        }
                                        
                                    } else {
                                        cout << "Erro SINTATICO! Diretiva \"" << command << "\" com a quantidade de operandos invalida "
                                             << "(linha " << line_count << ")." << endl;    
                                    }
                                } else if (command == "SPACE") {    // SPACE supports 0/1 params
                                    Param defined_param;
                                    defined_param.type = ParamType::Immediate; defined_param.value = 0;
                                    
                                    direc.operands.emplace_back(defined_param);
                                    is_valid = true;
                                } else if (TD.at(command).qtd_operands != param.size()) {
                                    cout << "Erro SINTATICO! Diretiva \"" << command << "\" com a quantidade de operandos invalida "
                                         << "(linha " << line_count << ")." << endl;
                                }

                                if (is_valid) {
                                    data_table.emplace_back(direc);
                                }
                            } else {
                                cout << "Erro SINTATICO! Diretiva invalida \"" << command << "\" "
                                     << "(linha " << line_count << ")." << endl;
                            }
                        } else {
                            cout << "Erro SEMANTICO! Rotulo repetido \"" << label << "\" "
                                 << "(linha " << line_count << ")." << endl;
                        }

                    } else if (command != "SECTION") {  // n eh transicao
                        cout << "Erro SINTATICO! Rotulo ausente "
                             << "(linha " << line_count << ")." << endl;
                    }
                } else if (!command.empty() && command != "SECTION") {  // sem rotulo e n eh transicao
                    cout << "Erro SEMANTICO! Diretiva/Instrucao na secao errada \"" << command << "\" "
                         << "(linha " << line_count << ")." << endl;
                }
                if (!is_valid) command.clear();

            } else if (currentMajorState == MajorState::SecondPass) {  // generate text/data table, check instr's params, check labels on TS
                if (currentSection == Section::Text) {
                    bool is_valid = true;   // valid command?
                    if (!command.empty()) {
                        if (TI.count(command) > 0) {
                            Instruction instr = TI.at(command);
                            instr.label = label;
                            vector<pair<string, int>> inc_param;        // parameters to include (label, imediate)

                            // determine if params have increment and put this on s_param vector
                            for (unsigned i = 0; i < param.size(); i++) {
                                pair<string, int> n_param;      // new parameter
                                n_param.first = "", n_param.second = 0;
                                if (!is_number(param[i])) { // label
                                    if (i + 2 < param.size() && param[i+1] == "+" && is_number(param[i+2])) {     // increment
                                        n_param.first = param[i];
                                        n_param.second = stoi(param[i+2]);

                                        i += 2; // skip next 2
                                    } else {       
                                        n_param.first = param[i];
                                    }
                                } else if (command == "S_INPUT" || command == "S_OUTPUT") {       // accepts immediate as parameter
                                    int im = stoi(param[i]);
                                    n_param.second = im;
                                } else {
                                    cout << "Erro SINTATICO! Parametro invalido \"" << param[i] << "\" "
                                         << "(linha " << line_count << ")." << endl;
                                    is_valid = false;
                                    break;
                                }

                                if (is_valid) inc_param.emplace_back(n_param);
                            }

                            if (instr.qtd_operands == inc_param.size()) {
                                if (instr.qtd_operands > 0) {  // has param
                                    for (unsigned i = 0; i < inc_param.size(); i++) {  // check param on TS
                                        Param defined_param;

                                        if (inc_param[i].first.size()) {    // has label
                                            if (TS.count(inc_param[i].first) > 0) {
                                                defined_param.type = ParamType::EndMem; 
                                                defined_param.value = TS[inc_param[i].first];
                                                defined_param.offset = inc_param[i].second;
                                                
                                                instr.operands.emplace_back(defined_param);
                                            } else {
                                                cout << "Erro SEMANTICO! Rotulo ausente \"" << inc_param[i].first << "\" "
                                                    << "(linha " << line_count << ")." << endl;
                                                is_valid = false;
                                            }
                                        } else {    // imediate
                                            defined_param.type = ParamType::Immediate;
                                            defined_param.value = inc_param[i].second;
                                            defined_param.offset = 0;
                                            
                                            instr.operands.emplace_back(defined_param);
                                        }
                                    }
                                    if (is_valid) text_table.push_back(instr);

                                } else {  // instr without param
                                    text_table.push_back(instr);
                                }
                            } else {
                                cout << "Erro SINTATICO! Instrucao \"" << command << "\" com a quantidade de operandos incorreta "
                                     << "(linha " << line_count << ")." << endl;
                                is_valid = false;
                            }
                        } else if (TD.count(command) > 0) {  // (occurs if command is a directive)
                            cout << "Erro SEMANTICO! Diretiva na secao errada \"" << command << "\" "
                                 << "(linha " << line_count << ")." << endl;
                            is_valid = false;
                        } else {
                            cout << "Erro SINTATICO! Instrucao invalida \"" << command << "\" "
                                 << "(linha " << line_count << ")." << endl;
                            is_valid = false;
                        }
                    } else {
                        cout << "Erro SINTATICO! Declaracao ausente "
                             << "(linha " << line_count << ")." << endl;
                        is_valid = false;
                    }
                    if (!is_valid) command.clear();
                }
            }

            // Update memory address
            if (TI.count(command) > 0) {
                end_count += TI.at(command).length;
            } else if (TD.count(command) > 0) {
                end_count += TD.at(command).length;
            }

            // Recovering state
            if (currentMajorState == MajorState::PassThrough) {
                currentMajorState = previousMajorState;
            }
        }

        if (currentMajorState == MajorState::FirstPass) {
            currentMajorState = MajorState::SecondPass;  // second pass
            previousMajorState = currentMajorState;

            currentSection = Section::None;

            in_file.clear();
            in_file.seekg(0, in_file.beg);

            line_count = 0;
            end_count = 0;

            if (!found_text) {
                cout << "Erro SINTATICO! Secao TEXT faltando." << endl;
                cout << "> Compilacao interrompida." << endl;

                fatal_error = true;
                stop = true;
            }
        } else {
            stop = true;
        }
    }

    if (!fatal_error) {
        assembly_table.text_table = text_table;
        assembly_table.data_table = data_table;
        assembly_table.TS = TS;
    } else {
        cout << "Erro! Nao foi possivel gerar as tabelas de compilacao." << endl;
    }

    return assembly_table;
}

bool assembly(ifstream &in_file, ofstream &out_file, int mode) {
    AssemblyTables tables = generate_tables(in_file);

    if (tables.empty()) return false;

    if (mode == 1) {
        _obj_pretty(out_file, tables.text_table, tables.data_table);
    } else {
        _obj_one_line(out_file, tables.text_table, tables.data_table);
    }

    return true;
}

bool translate_x86(ifstream &in_file, ofstream &out_file) {
    ofstream w_pre_file;
    w_pre_file.open("temp.PRE");
    if (!w_pre_file.is_open()) {
        cout << "Erro ao criar arquivo temporario de Pré-processamento" << endl;
        return false;
    }

    cout << "> Comecando Pre-processamento..." << endl;
    pre_process(in_file, w_pre_file);
    
    w_pre_file.close();
    
    ifstream r_pre_file;
    r_pre_file.open("temp.PRE");
    if (!r_pre_file.is_open()) {
        cout << "Erro ao abrir arquivo temporario de Pre-processamento" << endl;
        return false;
    }

    cout << "> Gerando tabelas de montagem..." << endl;
    AssemblyTables tables = generate_tables(r_pre_file);

    if (tables.empty()) return false;
    
    cout << "> Iniciando traducao para assembly x86..." << endl;
    _obj_x86(out_file, tables);

    r_pre_file.close();

    return true;
}
