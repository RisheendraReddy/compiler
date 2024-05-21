#include <iostream>
#include <vector>
#include <map>
#include "lexer.h"
#include "compiler.h"
using namespace std;

string var_array[100];
LexicalAnalyzer lexAnalyzer;



struct InstructionNode* parse_for_init();

struct InstructionNode* parse_for_cond();

struct InstructionNode* parse_for_post_action();

ArithmeticOperatorType parse_operator_token(TokenType token_type);

ConditionalOperatorType parse_condition_token(TokenType token_type);

struct InstructionNode* parse_statement_while();

struct InstructionNode* parse_statement_if();

struct InstructionNode* parse_program_body();

struct InstructionNode* parse_statement_list();

struct InstructionNode* parse_single_statement();

struct InstructionNode* parse_assign_statement();

struct InstructionNode* parse_output_statement();

struct InstructionNode* parse_input_statement();

struct InstructionNode* parse_switch_statement();

struct InstructionNode* parse_for_statement();

struct InstructionNode* parse_case_list(Token token_var, InstructionNode* skip_node);

struct InstructionNode* modify_node_with_token(InstructionNode* temp_node, Token token);

void map_var_to_index(Token token);

ArithmeticOperatorType parse_operator_token(TokenType token_type) {
    switch (token_type) {
        case PLUS:   return OPERATOR_PLUS;
        case MINUS:  return OPERATOR_MINUS;
        case MULT:   return OPERATOR_MULT;
        case DIV:    return OPERATOR_DIV;
        default:     return OPERATOR_NONE; 
    }
}

ConditionalOperatorType parse_condition_token(TokenType token_type) {
    switch (token_type) {
        case LESS:      return CONDITION_LESS;
        case GREATER:   return CONDITION_GREATER;
        case NOTEQUAL:  return CONDITION_NOTEQUAL;
    }
}

map<string, int> var_index_map;
void map_var_to_index(Token token) {
    if (var_index_map.find(token.lexeme) == var_index_map.end()) {
        var_index_map[token.lexeme] = next_available;
        mem[next_available] = stoi(token.lexeme);
        next_available++;
    }
}

InstructionNode* parse_program_body()
{
    lexAnalyzer.GetToken();
    InstructionNode* temp_nodes = parse_statement_list();
    lexAnalyzer.GetToken();
    return temp_nodes;
}

InstructionNode* modify_node_with_token(InstructionNode* temp_node, Token token)
{
    switch (token.token_type) {
        case PLUS:
        case MINUS:
        case MULT:
        case DIV:
            temp_node->assign_inst.op = parse_operator_token(token.token_type);
            break;
        case LESS:
        case GREATER:
        case NOTEQUAL:
            temp_node->cjmp_inst.condition_op = parse_condition_token(token.token_type);
            break;
        default:
            break;
    }
    return temp_node;
}

InstructionNode* parse_generate_intermediate_representation()
{
    
    Token token = lexAnalyzer.GetToken();
   
   
    while (token.token_type != SEMICOLON) {
        if (token.token_type == ID) {
            var_index_map[token.lexeme] = next_available;
            mem[next_available] = 0;
            next_available++;
        }
        token = lexAnalyzer.GetToken();
    }

    InstructionNode* temp_nodes = parse_program_body();

    token = lexAnalyzer.GetToken();
    while (token.token_type == NUM) {
        inputs.push_back(stoi(token.lexeme));
        token = lexAnalyzer.GetToken();
    }

    lexAnalyzer.GetToken();
    return temp_nodes;
}

InstructionNode* parse_single_statement()
{
    InstructionNode* start = new InstructionNode;
    Token temp_token = lexAnalyzer.peek(1);
    if(temp_token.token_type == ID)
        start = parse_assign_statement();
    else if(temp_token.token_type == OUTPUT)
        start = parse_output_statement();
    else if(temp_token.token_type == INPUT)
        start = parse_input_statement();
    else if(temp_token.token_type == WHILE)
    {
        start = parse_statement_while();
        start->next = parse_program_body();
        InstructionNode* jump = new InstructionNode;
        jump->type = JMP;
        jump->jmp_inst.target = start;
        InstructionNode* temp = start->next;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = jump;
        start->cjmp_inst.target = jump;
        InstructionNode* skip_node = new InstructionNode;
        skip_node->type = NOOP;
        skip_node->next = NULL;
        jump->next = skip_node;
        start->cjmp_inst.target = skip_node;
    }
    else if(temp_token.token_type == IF)
        start = parse_statement_if();
    else if(temp_token.token_type == SWITCH)
        start = parse_switch_statement();
    else if(temp_token.token_type == FOR)
        start = parse_for_statement();

    return start;
}

InstructionNode* parse_statement_list()
{
    InstructionNode* start = parse_single_statement();
    InstructionNode* current = start;
    Token nextToken = lexAnalyzer.peek(1);

    
    InstructionNode* nextInst = nullptr;

    switch (nextToken.token_type) {
        case INPUT:
        case OUTPUT:
        case ID:
        case IF:
        case FOR:
        case SWITCH:
        case WHILE:
            nextInst = parse_statement_list();  
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = nextInst;
            break;
        default:
            break;
    }

    return start;
}


InstructionNode* parse_input_statement()
{
    InstructionNode* input_node = new InstructionNode;
    lexAnalyzer.GetToken(); // skip 'input'
    Token temp_token = lexAnalyzer.GetToken(); // variable
    input_node->type = IN;
    input_node->input_inst.var_index = var_index_map[temp_token.lexeme];
    lexAnalyzer.GetToken(); // skip ';'
    input_node->next = NULL;
    return input_node;
}

InstructionNode* parse_output_statement() {
    InstructionNode* output_node = new InstructionNode;
    lexAnalyzer.GetToken(); // skip 'output'
    Token temp_token = lexAnalyzer.GetToken(); // variable
    output_node->type = OUT;
    output_node->output_inst.var_index = var_index_map[temp_token.lexeme];
    lexAnalyzer.GetToken(); // skip ';'
    output_node->next = NULL;
    return output_node;
}

InstructionNode* parse_case_list(Token token_var, InstructionNode* skip_node)
{
    InstructionNode* start = nullptr;
    InstructionNode* current = nullptr;

    while (true) {
        InstructionNode* case_node = new InstructionNode;
        lexAnalyzer.GetToken(); // skip 'case'
        Token case_value = lexAnalyzer.GetToken(); // case value
        map_var_to_index(case_value);

        
        case_node->type = CJMP;
        
        case_node->cjmp_inst.operand1_index = var_index_map[token_var.lexeme];
       
        case_node->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
        
        
        case_node->cjmp_inst.operand2_index = var_index_map[case_value.lexeme];
       
        case_node->next = nullptr;

        lexAnalyzer.GetToken(); // skip ':'

        case_node->cjmp_inst.target = parse_program_body();
        InstructionNode* temp = case_node->cjmp_inst.target;
        while (temp->next != nullptr) {
            temp = temp->next;
        }
        temp->next = skip_node;

        if (start == nullptr) {
            start = case_node;
            current = case_node;
        } else {
            current->next = case_node;
            current = case_node;
        }

        Token nextToken = lexAnalyzer.peek(1);
        if (nextToken.token_type != CASE) {
            break;
        }
    }

    return start;
}

InstructionNode* parse_assign_statement()
{
    Token lhs_token = lexAnalyzer.GetToken(); // left-hand side variable

    lexAnalyzer.GetToken(); // skip '='

    InstructionNode* assign_node = new InstructionNode;
    assign_node->type = ASSIGN;
    assign_node->assign_inst.left_hand_side_index = var_index_map[lhs_token.lexeme];

    Token first_operand = lexAnalyzer.GetToken(); // first operand
    Token operator_token = lexAnalyzer.GetToken(); // operator

    if (first_operand.token_type == NUM) {
        map_var_to_index(first_operand);
    }
    assign_node->assign_inst.operand1_index = var_index_map[first_operand.lexeme];

    if (operator_token.token_type != SEMICOLON) {
        assign_node = modify_node_with_token(assign_node, operator_token);

        Token second_operand = lexAnalyzer.GetToken(); // second operand
        lexAnalyzer.GetToken(); // skip ';'

        if (second_operand.token_type == NUM) {
            map_var_to_index(second_operand);
        }
        assign_node->assign_inst.operand2_index = var_index_map[second_operand.lexeme];
    } else {
        assign_node->assign_inst.op = OPERATOR_NONE;
    }

    assign_node->next = NULL;

    return assign_node;
}

InstructionNode* parse_statement_if()
{
    InstructionNode* if_node = new InstructionNode;
    InstructionNode* skip_node = new InstructionNode;

    lexAnalyzer.GetToken(); // skip 'if'

    if_node->type = CJMP;

    Token first_operand = lexAnalyzer.GetToken(); // first operand
    
    if_node->cjmp_inst.operand1_index = var_index_map[first_operand.lexeme];
   
    Token condition_token = lexAnalyzer.GetToken(); // condition
  
    if_node = modify_node_with_token(if_node, condition_token);
   
    Token second_operand = lexAnalyzer.GetToken(); // second operand

    if (second_operand.token_type == NUM) {
        map_var_to_index(second_operand);
    }
    if_node->cjmp_inst.operand2_index = var_index_map[second_operand.lexeme];

    if_node->next = parse_program_body();

    skip_node->type = NOOP;
    skip_node->next = NULL;

    InstructionNode* temp = if_node->next;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = skip_node;

    if_node->cjmp_inst.target = skip_node;

    return if_node;
}

InstructionNode* parse_for_init()
{
    return parse_assign_statement();
}

InstructionNode* parse_for_cond()
{
    InstructionNode* conditional_jump = new InstructionNode;
    conditional_jump->type = CJMP;

    Token first_operand = lexAnalyzer.GetToken(); // first operand
    
    conditional_jump->cjmp_inst.operand1_index = var_index_map[first_operand.lexeme];
    
    Token condition_token = lexAnalyzer.GetToken(); // condition
    
    conditional_jump = modify_node_with_token(conditional_jump, condition_token);
   
    Token second_operand = lexAnalyzer.GetToken(); // second operand
    if (second_operand.token_type == NUM) {
        map_var_to_index(second_operand);
    }
    conditional_jump->cjmp_inst.operand2_index = var_index_map[second_operand.lexeme];
    conditional_jump->cjmp_inst.target = nullptr;

    lexAnalyzer.GetToken(); // skip ';'

    return conditional_jump;
}

InstructionNode* parse_for_post_action()
{
    InstructionNode* loop_end = parse_assign_statement();
    InstructionNode* jump_instruction = new InstructionNode;
    jump_instruction->type = JMP;
    loop_end->next = jump_instruction;

    return loop_end;
}

InstructionNode* parse_for_statement()
{
    InstructionNode* jump_instruction = new InstructionNode;
    InstructionNode* noop_instruction = new InstructionNode;
    InstructionNode* loop_start;
    InstructionNode* loop_end;

    jump_instruction->type = JMP;
    jump_instruction->next = noop_instruction;

    noop_instruction->type = NOOP;
    noop_instruction->next = nullptr;

    lexAnalyzer.GetToken(); // skip 'for'
    lexAnalyzer.GetToken(); // skip '('

    loop_start = parse_for_init();
    loop_start->next = parse_for_cond();
    loop_end = parse_for_post_action();

    lexAnalyzer.GetToken(); // skip ')'

    loop_end->next = parse_program_body();

    InstructionNode* temp = loop_end->next;
    while (temp->next != nullptr) {
        temp = temp->next;
    }
    temp->next = loop_start;

    jump_instruction->jmp_inst.target = loop_end;

    return loop_start;
}

InstructionNode* parse_switch_statement()
{
    InstructionNode* skip_node = new InstructionNode;
    skip_node->type = NOOP;
    skip_node->next = NULL;
    InstructionNode* start;
   
    lexAnalyzer.GetToken();
    Token var_token = lexAnalyzer.GetToken();
    lexAnalyzer.GetToken();
    start = parse_case_list(var_token, skip_node);

    Token peek_token = lexAnalyzer.peek(1);
    if(peek_token.token_type == DEFAULT){
        InstructionNode* default_case = new InstructionNode;
        lexAnalyzer.GetToken();
        lexAnalyzer.GetToken();
        default_case = parse_program_body();
        InstructionNode* temp = start;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = default_case;
        temp = default_case;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = skip_node;
    }
    else if(peek_token.token_type == RBRACE){
        InstructionNode* temp = start;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = skip_node;
    }

    lexAnalyzer.GetToken(); // skip '}'
   
    return start;
}

InstructionNode* parse_statement_while() {
   
    InstructionNode* while_node = new InstructionNode;
   
    while_node->type = CJMP; // Set node type to conditional jump

    lexAnalyzer.GetToken(); // Skip 'while' keyword
    
   
    Token var_token = lexAnalyzer.GetToken(); // Get the variable token
  
    while_node->cjmp_inst.operand1_index = var_index_map[var_token.lexeme]; // Map index

    
    Token condition_token = lexAnalyzer.GetToken(); // Get condition token
   
    while_node = modify_node_with_token(while_node, condition_token); // Modify node with token

    Token num_token = lexAnalyzer.GetToken(); // Get number token
    if (num_token.token_type == NUM) {
        map_var_to_index(num_token); // Update index if token is a number
    }

    while_node->cjmp_inst.operand2_index = var_index_map[num_token.lexeme]; // Set second operand index

    return while_node; 
}
