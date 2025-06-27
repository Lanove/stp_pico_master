const mysql = require('mysql2/promise');
const fs = require('fs');
const path = require('path');
require('dotenv').config({ path: path.resolve(__dirname, '../.env') });

// Log environment variables for debugging (remove in production)
console.log('Using database configuration:');
console.log('DB Host:', process.env.DB_HOST || 'localhost');
console.log('DB User:', process.env.DB_USER || 'root');
console.log('DB Password is set:', process.env.DB_PASSWORD ? 'Yes' : 'No');
console.log('DB Name:', process.env.DB_NAME || 'loadbank_db');

async function setupDatabase() {
  let connection;

  try {
    // First connect without specifying a database
    connection = await mysql.createConnection({
      host: process.env.DB_HOST || 'localhost',
      user: process.env.DB_USER || 'root',
      password: process.env.DB_PASSWORD || 'lpkojihu', // Fallback to hardcoded password if not in env
    });

    console.log('Connected to MySQL server successfully.');

    // Read the SQL schema file
    const schemaPath = path.join(__dirname, 'config', 'schema.sql');
    const schemaSql = fs.readFileSync(schemaPath, 'utf8');

    // Split the SQL commands by semicolon to execute them one by one
    const commands = schemaSql
      .split(';')
      .filter(command => command.trim() !== '');

    for (const command of commands) {
      try {
        await connection.query(command + ';');
        console.log('Executed SQL command successfully.');
      } catch (err) {
        console.error(`Error executing SQL command: ${command}\n`, err);
      }
    }

    console.log('Database setup completed successfully!');
  } catch (err) {
    console.error('Error setting up database:', err);
  } finally {
    if (connection) {
      await connection.end();
      console.log('Database connection closed.');
    }
  }
}

setupDatabase();