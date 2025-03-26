# 🐬 LuaSQL MySQL Prepared Statements Extension

## 📖 Overview

LuaSQL MySQL Prepared Statements Extension is a Lua-C binding designed to provide a lightweight and efficient way to use MySQL prepared statements directly in Lua 5.4. This extension enhances database operations by offering a safer and faster approach to interacting with MySQL databases.

### Key Highlights:
- **Safer and faster database operations** using prepared statements.
- **Prevention of SQL injection risks** through parameterized queries.
- **Example usage included** for quick integration.

## 🔧 Features

1. **MySQL Connection Pooling**  
    Efficiently manage database connections for better performance.

2. **Parameterized Queries**  
    Simplify query execution with dynamic parameters.

3. **Safe Data Insertion and Retrieval**  
    Ensure data integrity and security during database operations.

4. **Minimal, Easy-to-Use API**  
    A straightforward API designed for simplicity and productivity.

5. **Lua 5.4 Compatibility**  
    Fully compatible with Lua 5.4, written in C for optimal performance.

## 🛠️ Core Functions

The extension includes five custom LuaSQL-like functions:

- `connect()`  
  Establish a connection to the MySQL database.

- `prepare()`  
  Prepare SQL statements for execution.

- `bind()`  
  Bind parameters to the prepared statement.

- `execute()`  
  Execute the prepared statement.

- `fetch()`  
  Retrieve results from the executed statement.

---
## output screenshots
![alt text](<Screenshot from 2025-03-26 15-40-19.png>)

