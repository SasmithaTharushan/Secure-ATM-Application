#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_PIN_ATTEMPTS 3
#define MAX_ACCOUNTS 3
#define MAX_NAME_LENGTH 50
#define MAX_ACCOUNT_NUMBER_LENGTH 20
#define BUFFER_SIZE 100
#define MIN_BALANCE 500.0  // Minimum balance that must be maintained
#define MAX_TRANSACTION_AMOUNT 50000.0  // Maximum amount per transaction

// Structure to store account information
struct Account {
    char name[MAX_NAME_LENGTH];
    char account_number[MAX_ACCOUNT_NUMBER_LENGTH];
    double balance;
};

// Global variables
struct Account accounts[MAX_ACCOUNTS];
int current_account = 0;
const int CORRECT_PIN = 1234;  // Initialize PIN

// Function prototypes
void initializeAccounts(void);
bool validatePin(void);
void clearInputBuffer(void);
bool getSecureInput(char *buffer, size_t size);
bool isValidAmount(double amount);
void checkBalance(void);
bool withdrawal(void);
bool deposit(void);
bool transfer(void);
void displayTransactionHistory(void);

// Transaction history structure
struct Transaction {
    char type[20];
    double amount;
    char details[100];
};

struct Transaction transactions[100];
int transaction_count = 0;

// Function to safely get numeric input
bool getNumericInput(char *buffer, size_t size) {
    if (!getSecureInput(buffer, size)) {
        return false;
    }
    
    // Verify that input contains only digits
    for (size_t i = 0; i < strlen(buffer); i++) {
        if (!isdigit(buffer[i])) {
            return false;
        }
    }
    return true;
}

// Function to safely get input with buffer overflow protection
bool getSecureInput(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return false;
    }
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    } else {
        clearInputBuffer();
        return false;
    }
    return true;
}

// Function to clear input buffer
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Initialize sample accounts
void initializeAccounts(void) {
    // Account 1
    strncpy(accounts[0].name, "John Doe", MAX_NAME_LENGTH - 1);
    strncpy(accounts[0].account_number, "1000001", MAX_ACCOUNT_NUMBER_LENGTH - 1);
    accounts[0].balance = 10000.0;

    // Account 2
    strncpy(accounts[1].name, "Jane Smith", MAX_NAME_LENGTH - 1);
    strncpy(accounts[1].account_number, "1000002", MAX_ACCOUNT_NUMBER_LENGTH - 1);
    accounts[1].balance = 15000.0;

    // Account 3
    strncpy(accounts[2].name, "Bob Johnson", MAX_NAME_LENGTH - 1);
    strncpy(accounts[2].account_number, "1000003", MAX_ACCOUNT_NUMBER_LENGTH - 1);
    accounts[2].balance = 20000.0;
}

// Validate PIN with limited attempts
bool validatePin(void) {
    char pin_input[BUFFER_SIZE];
    int attempts = 0;
    
    while (attempts < MAX_PIN_ATTEMPTS) {
        printf("\nEnter PIN: ");
        if (!getNumericInput(pin_input, BUFFER_SIZE)) {
            printf("Invalid input! Please enter numbers only.\n");
            continue;
        }
        
        if (atoi(pin_input) == CORRECT_PIN) {
            return true;
        }
        
        attempts++;
        printf("Incorrect PIN. %d attempts remaining.\n", MAX_PIN_ATTEMPTS - attempts);
    }
    
    printf("Too many incorrect attempts. Card blocked.\n");
    return false;
}

// Validate transaction amount
bool isValidAmount(double amount) {
    return amount > 0 && amount <= MAX_TRANSACTION_AMOUNT;
}

// Check balance
void checkBalance(void) {
    printf("\nAccount Holder: %s", accounts[current_account].name);
    printf("\nAccount Number: %s", accounts[current_account].account_number);
    printf("\nCurrent Balance: $%.2f\n", accounts[current_account].balance);
    
    // Record transaction
    strncpy(transactions[transaction_count].type, "Balance Check", 19);
    transactions[transaction_count].amount = 0;
    snprintf(transactions[transaction_count].details, 99, "Balance inquiry - Current balance: $%.2f", 
             accounts[current_account].balance);
    transaction_count++;
}

// Withdraw money
bool withdrawal(void) {
    char amount_str[BUFFER_SIZE];
    double amount;
    
    printf("\nEnter amount to withdraw: $");
    if (!getNumericInput(amount_str, BUFFER_SIZE)) {
        printf("Invalid input!\n");
        return false;
    }
    
    amount = atof(amount_str);
    
    if (!isValidAmount(amount)) {
        printf("Invalid amount! Amount must be between $0 and $%.2f\n", MAX_TRANSACTION_AMOUNT);
        return false;
    }
    
    if (amount > accounts[current_account].balance - MIN_BALANCE) {
        printf("Insufficient funds! Minimum balance of $%.2f must be maintained.\n", MIN_BALANCE);
        return false;
    }
    
    accounts[current_account].balance -= amount;
    printf("Withdrawal successful. Current balance: $%.2f\n", accounts[current_account].balance);
    
    // Record transaction
    strncpy(transactions[transaction_count].type, "Withdrawal", 19);
    transactions[transaction_count].amount = amount;
    snprintf(transactions[transaction_count].details, 99, "Withdrew $%.2f", amount);
    transaction_count++;
    
    return true;
}

// Deposit money
bool deposit(void) {
    char amount_str[BUFFER_SIZE];
    double amount;
    
    printf("\nEnter amount to deposit: $");
    if (!getNumericInput(amount_str, BUFFER_SIZE)) {
        printf("Invalid input!\n");
        return false;
    }
    
    amount = atof(amount_str);
    
    if (!isValidAmount(amount)) {
        printf("Invalid amount! Amount must be between $0 and $%.2f\n", MAX_TRANSACTION_AMOUNT);
        return false;
    }
    
    accounts[current_account].balance += amount;
    printf("Deposit successful. Current balance: $%.2f\n", accounts[current_account].balance);
    
    // Record transaction
    strncpy(transactions[transaction_count].type, "Deposit", 19);
    transactions[transaction_count].amount = amount;
    snprintf(transactions[transaction_count].details, 99, "Deposited $%.2f", amount);
    transaction_count++;
    
    return true;
}

// Transfer money
bool transfer(void) {
    char amount_str[BUFFER_SIZE];
    char account_str[BUFFER_SIZE];
    double amount;
    int target_account;
    
    printf("\nEnter target account number: ");
    if (!getNumericInput(account_str, BUFFER_SIZE)) {
        printf("Invalid account number!\n");
        return false;
    }
    
    // Find target account
    target_account = -1;
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        if (strcmp(accounts[i].account_number, account_str) == 0 && i != current_account) {
            target_account = i;
            break;
        }
    }
    
    if (target_account == -1) {
        printf("Account not found!\n");
        return false;
    }
    
    printf("Enter amount to transfer: $");
    if (!getNumericInput(amount_str, BUFFER_SIZE)) {
        printf("Invalid input!\n");
        return false;
    }
    
    amount = atof(amount_str);
    
    if (!isValidAmount(amount)) {
        printf("Invalid amount! Amount must be between $0 and $%.2f\n", MAX_TRANSACTION_AMOUNT);
        return false;
    }
    
    if (amount > accounts[current_account].balance - MIN_BALANCE) {
        printf("Insufficient funds! Minimum balance of $%.2f must be maintained.\n", MIN_BALANCE);
        return false;
    }
    
    accounts[current_account].balance -= amount;
    accounts[target_account].balance += amount;
    
    printf("Transfer successful!\n");
    printf("Current balance: $%.2f\n", accounts[current_account].balance);
    
    // Record transaction
    strncpy(transactions[transaction_count].type, "Transfer", 19);
    transactions[transaction_count].amount = amount;
    snprintf(transactions[transaction_count].details, 99, 
             "Transferred $%.2f to account %s", amount, accounts[target_account].account_number);
    transaction_count++;
    
    return true;
}

// Display transaction history
void displayTransactionHistory(void) {
    printf("\nTransaction History:\n");
    printf("-------------------\n");
    for (int i = 0; i < transaction_count; i++) {
        printf("%d. %s: %s\n", i + 1, transactions[i].type, transactions[i].details);
    }
}

int main(void) {
    char choice[BUFFER_SIZE];
    bool continue_transaction = true;
    
    initializeAccounts();
    
    printf("Welcome to SecureATM\n");
    
    if (!validatePin()) {
        return 1;
    }
    
    while (continue_transaction) {
        printf("\n=== ATM Menu ===\n");
        printf("1. Check Balance\n");
        printf("2. Withdraw Money\n");
        printf("3. Deposit Money\n");
        printf("4. Transfer Money\n");
        printf("5. Transaction History\n");
        printf("6. Exit\n");
        printf("Enter choice (1-6): ");
        
        if (!getNumericInput(choice, BUFFER_SIZE)) {
            printf("Invalid input! Please try again.\n");
            continue;
        }
        
        switch (atoi(choice)) {
            case 1:
                checkBalance();
                break;
            case 2:
                withdrawal();
                break;
            case 3:
                deposit();
                break;
            case 4:p
                transfer();
                break;
            case 5:
                displayTransactionHistory();
                break;
            case 6:
                continue_transaction = false;
                printf("\nThank you for using SecureATM. Goodbye!\n");
                break;
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
    
    return 0;
}