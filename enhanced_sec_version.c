#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// Security Constants
#define MAX_PIN_ATTEMPTS 3
#define PIN_LENGTH 4
#define SALT_LENGTH 16
#define HASH_LENGTH 64  // SHA-256 output length
#define SESSION_TIMEOUT_SECONDS 300  // 5 minutes
#define MAX_DAILY_WITHDRAWAL 5000.0
#define MAX_DAILY_TRANSFER 10000.0

// Buffer and Account Constants
#define MAX_ACCOUNTS 3
#define MAX_NAME_LENGTH 50
#define MAX_ACCOUNT_NUMBER_LENGTH 20
#define BUFFER_SIZE 100
#define MIN_BALANCE 500.0
#define MAX_TRANSACTION_AMOUNT 50000.0
#define MAX_TRANSACTIONS_PER_SESSION 5
#define MAX_TRANSACTION_HISTORY 100

// Error Codes
#define SUCCESS 0
#define ERROR_INVALID_INPUT -1
#define ERROR_INSUFFICIENT_FUNDS -2
#define ERROR_ACCOUNT_LOCKED -3
#define ERROR_SESSION_TIMEOUT -4
#define ERROR_DAILY_LIMIT_EXCEEDED -5

// Structure for secure PIN storage
struct SecurePIN {
    unsigned char salt[SALT_LENGTH];
    unsigned char hash[HASH_LENGTH];
};

// Structure for session management
struct Session {
    time_t last_activity;
    bool is_active;
    double daily_withdrawal;
    double daily_transfer;
    int transaction_count;
    char session_id[33];  // 32 hex chars + null terminator
};

// Enhanced Account structure
struct Account {
    char name[MAX_NAME_LENGTH];
    char account_number[MAX_ACCOUNT_NUMBER_LENGTH];
    double balance;
    struct SecurePIN pin;
    bool is_locked;
    time_t lock_until;
    int failed_attempts;
    struct Transaction {
        time_t timestamp;
        char type[20];
        double amount;
        char details[100];
        char transaction_id[33];
    } transaction_history[MAX_TRANSACTION_HISTORY];
    int transaction_count;
};

// Global variables
struct Account accounts[MAX_ACCOUNTS];
struct Session current_session;
int current_account = -1;

// Function prototypes
void initializeSystem(void);
int validatePIN(const char* input_pin);
void hashPIN(const char* pin, unsigned char* salt, unsigned char* hash);
bool generateSecureSessionId(char* session_id);
bool isSessionValid(void);
void updateSessionActivity(void);
void logTransaction(const char* type, double amount, const char* details);
void logSecurityEvent(const char* event, const char* details);
int performSecureWithdrawal(double amount);
int performSecureTransfer(int target_account, double amount);
void encryptSensitiveData(char* data);
void decryptSensitiveData(char* data);
bool validateInputFormat(const char* input, const char* format);
void sanitizeInput(char* input);
void clearSensitiveData(void* ptr, size_t size);

// Initialize OpenSSL
void initializeOpenSSL(void) {
    OpenSSL_add_all_algorithms();
}

// Clean up OpenSSL
void cleanupOpenSSL(void) {
    EVP_cleanup();
}

// Secure random number generation
int generateSecureRandom(unsigned char* buffer, size_t length) {
    return RAND_bytes(buffer, length);
}

// Hash PIN with salt using SHA-256
void hashPIN(const char* pin, unsigned char* salt, unsigned char* hash) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* sha256 = EVP_sha256();
    
    EVP_DigestInit_ex(ctx, sha256, NULL);
    EVP_DigestUpdate(ctx, salt, SALT_LENGTH);
    EVP_DigestUpdate(ctx, pin, strlen(pin));
    
    unsigned int hash_len;
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    
    EVP_MD_CTX_free(ctx);
}

// Generate secure session ID
bool generateSecureSessionId(char* session_id) {
    unsigned char random_bytes[16];
    if (generateSecureRandom(random_bytes, sizeof(random_bytes)) != 1) {
        return false;
    }
    
    for (int i = 0; i < 16; i++) {
        sprintf(&session_id[i * 2], "%02x", random_bytes[i]);
    }
    session_id[32] = '\0';
    return true;
}

// Validate session timeout and transaction limits
bool isSessionValid(void) {
    time_t current_time = time(NULL);
    
    if (!current_session.is_active ||
        (current_time - current_session.last_activity) > SESSION_TIMEOUT_SECONDS ||
        current_session.transaction_count >= MAX_TRANSACTIONS_PER_SESSION) {
        return false;
    }
    
    return true;
}

// Update session activity timestamp
void updateSessionActivity(void) {
    current_session.last_activity = time(NULL);
}

// Log transaction with secure transaction ID
void logTransaction(const char* type, double amount, const char* details) {
    struct Account* acc = &accounts[current_account];
    struct Transaction* trans = &acc->transaction_history[acc->transaction_count % MAX_TRANSACTION_HISTORY];
    
    trans->timestamp = time(NULL);
    strncpy(trans->type, type, sizeof(trans->type) - 1);
    trans->amount = amount;
    strncpy(trans->details, details, sizeof(trans->details) - 1);
    generateSecureSessionId(trans->transaction_id);
    
    acc->transaction_count++;
    current_session.transaction_count++;
}

// Secure withdrawal with daily limit check
int performSecureWithdrawal(double amount) {
    if (current_session.daily_withdrawal + amount > MAX_DAILY_WITHDRAWAL) {
        return ERROR_DAILY_LIMIT_EXCEEDED;
    }
    
    if (amount > accounts[current_account].balance - MIN_BALANCE) {
        return ERROR_INSUFFICIENT_FUNDS;
    }
    
    accounts[current_account].balance -= amount;
    current_session.daily_withdrawal += amount;
    
    char details[100];
    snprintf(details, sizeof(details), "Withdrawal of $%.2f", amount);
    logTransaction("WITHDRAWAL", amount, details);
    
    return SUCCESS;
}

// Secure transfer with daily limit check
int performSecureTransfer(int target_account, double amount) {
    if (current_session.daily_transfer + amount > MAX_DAILY_TRANSFER) {
        return ERROR_DAILY_LIMIT_EXCEEDED;
    }
    
    if (amount > accounts[current_account].balance - MIN_BALANCE) {
        return ERROR_INSUFFICIENT_FUNDS;
    }
    
    accounts[current_account].balance -= amount;
    accounts[target_account].balance += amount;
    current_session.daily_transfer += amount;
    
    char details[100];
    snprintf(details, sizeof(details), 
             "Transfer of $%.2f to account %s",
             amount, accounts[target_account].account_number);
    logTransaction("TRANSFER", amount, details);
    
    return SUCCESS;
}

// Input validation with format checking
bool validateInputFormat(const char* input, const char* format) {
    // Implement regex or pattern matching based on format
    // For simplicity, basic validation shown here
    if (strcmp(format, "PIN") == 0) {
        if (strlen(input) != PIN_LENGTH) return false;
        for (int i = 0; i < PIN_LENGTH; i++) {
            if (!isdigit(input[i])) return false;
        }
        return true;
    }
    // Add more format validations as needed
    return false;
}

// Sanitize input to prevent injection attacks
void sanitizeInput(char* input) {
    char* src = input;
    char* dst = input;
    
    while (*src) {
        if (isalnum(*src) || *src == ' ' || *src == '.' || *src == '-') {
            *dst = *src;
            dst++;
        }
        src++;
    }
    *dst = '\0';
}

// Clear sensitive data from memory
void clearSensitiveData(void* ptr, size_t size) {
    volatile unsigned char* p = ptr;
    while (size--) {
        *p++ = 0;
    }
}

int main(void) {
    char input_buffer[BUFFER_SIZE];
    int result;
    
    initializeOpenSSL();
    initializeSystem();
    
    printf("Welcome to SecureATM\n");
    
    // Main ATM loop with enhanced security
    while (1) {
        if (!isSessionValid()) {
            printf("Session expired. Please start a new session.\n");
            break;
        }
        
        // ... (rest of the main loop implementation)
        
        updateSessionActivity();
    }
    
    // Cleanup
    clearSensitiveData(&current_session, sizeof(current_session));
    cleanupOpenSSL();
    
    return 0;
}

// Additional security-focused functions would be implemented here:
// - Encryption/decryption of sensitive data
// - Secure key management
// - Audit logging
// - Error handling
// - Network security (if applicable)
// - Hardware security module (HSM) integration
// - Anti-tampering measures