#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <array>
#include <iomanip>
#include <wasi/api.h>
#include <iostream>
#include <fstream>
#include "errors.h"
#include "constants.h"
#include <algorithm>

extern "C" int getentropy(void* buffer, size_t length) {
    return __wasi_random_get(static_cast<uint8_t*>(buffer), length);
}

template<int lower, int upper>
int get_random_int() {
    static std::mt19937 rng((std::random_device())());
    static std::uniform_int_distribution<int> dist(lower, upper);
    return dist(rng);
}

template<int lower, int upper>
float get_random_float() {
    static std::mt19937 rng((std::random_device())());
    static std::uniform_real_distribution<float> dist(lower, upper);
    return dist(rng);
}

std::string get_random_string(int length = 1) {
    std::string result;
    while (length--) {
        int idx = get_random_int<0, 3>();
        result.push_back('a' + idx);
    }
    return result;
}


std::string get_test_case() {
    std::ostringstream out;
    static std::array<std::string, 33> instructions{
        "iconst", // 0
        "fconst", // 1

        "iload", // 2
        "fload", // 3
        "istore", // 4
        "fstore", // 5
        "val", // 6
        "par", // 7

        "iadd",
        "fadd",
        "isub",
        "fsub",
        "imul",
        "fmul",
        "idiv",
        "fdiv",
        "irem",
        "ineg",
        "fneg",
        "iand",
        "ior",
        "ieq",
        "feq",
        "ineq",
        "fneq",
        "ilt",
        "flt",
        "igt",
        "fgt",
        "ibnot",
        "i2f",
        "f2i",
        "top"
    };
    auto dist = []() {
        std::array<int, instructions.size()> dist;
        dist.fill(1);
        dist[0] = 50;
        dist[1] = 50;
        dist[4] = 50;
        dist[5] = 50;
        dist[6] = 30;
        dist[7] = 30;
        dist.back() = 50;
        return std::discrete_distribution<int>(dist.begin(), dist.end());
    }();

    static std::mt19937 rng((std::random_device())());

    int n_lines = 100;
    while (n_lines--) {
        int idx = dist(rng);

        out << instructions[idx];

        switch (idx) {
            case 0:
                out << ' ' << get_random_int<-10, 10>();
                break;
            case 1:
                out << ' ' << std::fixed << std::setprecision(3) << get_random_float<-10, 10>();
                break;
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                out << ' ' << get_random_string();
        }

    
        out << '\n';
    }




    return out.str();
}


namespace ans {
class StackFrame {
    int opStackMaxSize; // max size of operand stack
    int localVarSpaceSize; // size of local variable array
public:
    StackFrame();
    void run(std::string filename);
};

StackFrame::StackFrame() : opStackMaxSize(OPERAND_STACK_MAX_SIZE), localVarSpaceSize(LOCAL_VARIABLE_SPACE_SIZE) {}
int line_num;
enum Type {
	INT,
	FLOAT
};
struct Val {
	explicit Val(Type type, int i) {
		if (type == FLOAT) std::abort();
		this->type = type;
		this->i = i;
	}
	explicit Val(Type type, float f) {
		if (type == INT) std::abort();
		this->type = type;
		this->f = f;
	}
	int get_int() const {
		if (type == FLOAT) std::abort();
		return i;
	}
	float get_float() const {
		if (type == INT) std::abort();
		return f;
	}
	bool is_int() const {
		return type == INT;
	}
    bool is_float() const {
        return type == FLOAT;
    }
    void int2float() {
        if (type == FLOAT) std::abort();
        type = FLOAT;
        f = i;
    }
    void float2int() {
        if (type == INT) std::abort();
        type = INT;
        i = f;
    } 
    friend std::ostream& operator<<(std::ostream& stream, Val const& val) {
        if (val.is_int())
            return stream << val.i;
        else 
            return stream << val.f;
    }
private:
	Type type;
	union {
		int i;
		float f;
	};
};
struct AVLTree {
		
	
public:
	AVLTree() : root{nullptr}, size{0} {}
	~AVLTree() {
		if (root) delete root;
	}

	void insert(std::string name, Val value) {
        Node* old = findNodeByName(name);
        if (old == nullptr) {
            if (size == LOCAL_VARIABLE_SPACE_SIZE / 2) throw LocalSpaceFull(line_num);
		    root = insertNode(root, NodeKey{name, value});
            ++size;
        }
        else old->key.value = value;
	}
	
    Val findValByName(std::string name) {
        Node* node = findNodeByName(name);
        if (node == nullptr) throw UndefinedVariable(line_num);
        return node->key.value;
    }

	std::string findParentName(std::string name) {
		Node* parent = nullptr;
		Node* node = root;
		while (node != nullptr) {
			if (node->key.name == name) break;
			parent = node;
			node = name < node->key.name ? node->left : node->right;
		}
        if (node == nullptr) throw UndefinedVariable(line_num);
		return parent ? parent->key.name : "null";
	}
private:
    
	struct NodeKey {
		std::string name;
		Val value;
		bool operator<(NodeKey const& other) const {
			return name < other.name;
		}
	};
	struct Node {
		NodeKey key;
		Node *left;
		Node *right;
		int height;
		Node(NodeKey key) : key{key}, left{nullptr}, right{nullptr}, height{1} {}
		~Node() {
			if (left) delete left;
			if (right) delete right;
		}
		static int getHeight(Node* node) {
			if (node == nullptr) return 0;
			return node->height;
		}
		static int getBalance(Node* node) {
			if (node == nullptr) return 0;
			return Node::getHeight(node->left) - Node::getHeight(node->right);
		}
		
		static Node* rightRotate(Node *y) {
			Node *x = y->left;
			Node *T2 = x->right;
		
			x->right = y;
			y->left = T2;
		
			y->height = std::max(Node::getHeight(y->left),
							Node::getHeight(y->right)) + 1;
			x->height = std::max(Node::getHeight(x->left),
							Node::getHeight(x->right)) + 1;

			return x;
		}
		
		static Node* leftRotate(Node *x) {
			Node *y = x->right;
			Node *T2 = y->left;
		
			y->left = x;
			x->right = T2;
		
			x->height = std::max(Node::getHeight(x->left),   
							Node::getHeight(x->right)) + 1;
			y->height = std::max(Node::getHeight(y->left),
							Node::getHeight(y->right)) + 1;
		
			return y;
		}
	};
    Node* findNodeByName(std::string name) {
		Node* node = root;
		while (node != nullptr) {
			if (node->key.name == name) break;
			node = name < node->key.name ? node->left : node->right;
		}
		return node;
	}
	
	static Node* insertNode(Node* node, NodeKey key) {
		/* 1. Perform the normal BST insertion */
		if (node == nullptr) return new Node(key);
	
		if (key < node->key)
			node->left = insertNode(node->left, key);
		else if (node->key < key)
			node->right = insertNode(node->right, key);

	
		node->height = 1 + std::max(Node::getHeight(node->left), Node::getHeight(node->right));

		int balance = Node::getBalance(node);

		if (balance > 1 && key < node->left->key)
			return Node::rightRotate(node);
	
		if (balance < -1 && node->right->key < key)
			return Node::leftRotate(node);
	
		if (balance > 1 && node->left->key < key)
		{
			node->left = Node::leftRotate(node->left);
			return Node::rightRotate(node);
		}
	
		if (balance < -1 && key < node->right->key)
		{
			node->right = Node::rightRotate(node->right);
			return Node::leftRotate(node);
		}
	
		return node;
	}
	
	Node* root;
    int size;
	
};

struct Stack {
    std::vector<Val> stack;

    Val pop() {
        if (stack.empty()) throw StackEmpty(line_num);
        Val top = stack.back();
        stack.pop_back();
        return top;
    }
    void push(Val val) {
        if (stack.size() == OPERAND_STACK_MAX_SIZE / 2) throw StackFull(line_num);
        stack.push_back(val);
    }
    Val peek() const {
        if (stack.empty()) throw StackEmpty(line_num);
        return stack.back();
    }
    void clear() {
        stack.clear();
    }
};

Val do_int_binaryop(Val const& l, Val const& r, std::string inst) {
    if (!l.is_int() || !r.is_int()) throw TypeMisMatch(line_num);
    int const a = l.get_int();
    int const b = r.get_int();
    int res;
    if (inst == "iadd") {
        res = a + b;
    }
    else if (inst == "isub") {
        res = a - b;
    }
    else if (inst == "imul") {
        res = a * b;
    }
    else if (inst == "idiv") {
        if (b == 0) throw DivideByZero(line_num);
        res = a / b;
    }
    else if (inst == "irem") {
        if (b == 0) throw DivideByZero(line_num);
        res = a - (a / b) * b;
    }
    else if (inst == "iand") {
        res = a & b;
    }
    else if (inst == "ior") {
        res = a | b;
    }
    else if (inst == "ieq") {
        res = a == b ? 1 : 0;
    }
    else if (inst == "ineq") {
        res = a != b ? 1 : 0;
    }
    else if (inst == "ilt") {
        res = a < b ? 1 : 0;
    }
    else if (inst == "igt") {
        res = a > b ? 1 : 0;
    }
    return Val{INT, res};
}

Val do_float_binaryop(Val const& l, Val const& r, std::string inst) {
    float const a = l.is_int() ? l.get_int() : l.get_float();
    float const b = r.is_int() ? r.get_int() : r.get_float();

    float res;

    if (inst == "fadd") {
        res = a + b;
    }
    else if (inst == "fsub") {
        res = a - b;
    }
    else if (inst == "fmul") {
        res = a * b;
    }
    else if (inst == "fdiv") {
        if (b == 0) throw DivideByZero(line_num);
        res = a / b;
    }
    else {
        int res;
        if (inst == "feq") {
            res = a == b ? 1 : 0;
        }
        else if (inst == "fneq") {
            res = a != b ? 1 : 0;
        }
        else if (inst == "flt") {
            res = a < b ? 1 : 0;
        }
        else if (inst == "fgt") {
            res = a > b ? 1 : 0;
        }
        return Val{INT, res};
    }


    return Val{FLOAT, res};
}


void StackFrame::run(std::string filename) {
    std::ifstream file(filename);
    AVLTree tree;
    std::string inst;
    line_num = 1;
    int ival;
    float fval;
    std::string name;
    
    Stack stack;

    while (file >> inst) {
        if (inst == "iconst") {
            file >> ival;
            stack.push(Val{INT, ival});
        }
        else if (inst == "fconst") {
            file >> fval;
            stack.push(Val{FLOAT, fval});
        }
        else if (inst == "iload") {
            file >> name;
            Val val = tree.findValByName(name);
            if (!val.is_int()) throw TypeMisMatch(line_num);
            stack.push(val);
        }
        else if (inst == "fload") {
            file >> name;
            Val val = tree.findValByName(name);
            if (!val.is_float()) throw TypeMisMatch(line_num);
            stack.push(val);
        }
        else if (inst == "istore") {
            file >> name;
            Val val = stack.pop();
            if (!val.is_int()) throw TypeMisMatch(line_num);
            tree.insert(name, val);
        }
        else if (inst == "fstore") {
            file >> name;
            Val val = stack.pop();
            if (!val.is_float()) throw TypeMisMatch(line_num);
            tree.insert(name, val);
        }
        else if (inst == "i2f") {
            Val val = stack.pop();
            if (!val.is_int()) throw TypeMisMatch(line_num);
            val.int2float();
            stack.push(val);
        }
        else if (inst == "f2i") {
            Val val = stack.pop();
            if (!val.is_float()) throw TypeMisMatch(line_num);
            val.float2int();
            stack.push(val);
        }
        else if (inst == "top") {
            std::cout << stack.peek() << '\n';
        }
        else if (inst == "val") {
            file >> name;
            std::cout << tree.findValByName(name) << '\n';
        }
        else if (inst == "par") {
            file >> name;
            std::cout << tree.findParentName(name) << '\n';
        }
        else if (inst == "ibnot") {
            Val val = stack.pop();
            if (!val.is_int()) throw TypeMisMatch(line_num);
            stack.push(Val{INT, val.get_int() == 0 ? 1 : 0});
        }
        else if (inst == "ineg") {
            Val val = stack.pop();
            if (!val.is_int()) throw TypeMisMatch(line_num);
            stack.push(Val{INT, -val.get_int()});
        }
        else if (inst == "fneg") {
            Val val = stack.pop();
            if (!val.is_float()) val.int2float();
            stack.push(Val{FLOAT, -val.get_float()});
        }
        else {
            Val b = stack.pop();
            Val a = stack.pop();
            stack.push(inst.front() == 'i' ? do_int_binaryop(a, b, inst) : do_float_binaryop(a, b, inst));
        }

        ++line_num;
    }
}
}