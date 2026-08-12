-- ============================================================================
-- schema.sql
-- Normalized schema for the Bank Management System.
--   customers (1) --- (N) accounts (1) --- (N) transactions
-- Run this once to create the database and app user, or rely on
-- DatabaseManager::ensureSchema() to create the tables automatically
-- inside an already-existing database.
-- ============================================================================

CREATE DATABASE IF NOT EXISTS bank_management_system
    CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE bank_management_system;

CREATE TABLE IF NOT EXISTS customers (
    customer_id INT AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(100) NOT NULL,
    email       VARCHAR(100) NOT NULL UNIQUE,
    phone       VARCHAR(20)  NOT NULL,
    address     VARCHAR(255)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS accounts (
    account_id   INT AUTO_INCREMENT PRIMARY KEY,
    customer_id  INT NOT NULL,
    account_type VARCHAR(20) NOT NULL,          -- 'Savings' | 'Current'
    balance      DECIMAL(15,2) NOT NULL DEFAULT 0.00,
    pin_hash     VARCHAR(64) NOT NULL,
    FOREIGN KEY (customer_id) REFERENCES customers(customer_id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS transactions (
    transaction_id INT AUTO_INCREMENT PRIMARY KEY,
    account_id     INT NOT NULL,
    type           VARCHAR(20) NOT NULL,        -- DEPOSIT | WITHDRAWAL | TRANSFER_IN | TRANSFER_OUT
    amount         DECIMAL(15,2) NOT NULL,
    balance_after  DECIMAL(15,2) NOT NULL,
    txn_timestamp  VARCHAR(25) NOT NULL,
    remarks        VARCHAR(255),
    FOREIGN KEY (account_id) REFERENCES accounts(account_id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE INDEX idx_accounts_customer_id ON accounts(customer_id);
CREATE INDEX idx_transactions_account_id ON transactions(account_id);

-- Example dedicated application user (recommended over using root):
-- CREATE USER 'bank_app'@'localhost' IDENTIFIED BY 'change_me';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON bank_management_system.* TO 'bank_app'@'localhost';
-- FLUSH PRIVILEGES;
