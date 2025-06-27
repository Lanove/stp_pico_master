-- Create database if it doesn't exist
CREATE DATABASE IF NOT EXISTS loadbank_db;
USE loadbank_db;

-- Readings table
CREATE TABLE IF NOT EXISTS readings (
  id INT AUTO_INCREMENT PRIMARY KEY,
  voltage DECIMAL(10, 2) NOT NULL,
  current DECIMAL(10, 2) NOT NULL,
  power DECIMAL(10, 2) NOT NULL,
  energy DECIMAL(10, 2) NOT NULL,
  temperature INT,
  is_started BOOLEAN DEFAULT FALSE,
  time_now VARCHAR(8) DEFAULT '00:00:00',
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Settings table
CREATE TABLE IF NOT EXISTS settings (
  id INT AUTO_INCREMENT PRIMARY KEY,
  setpoint INT NOT NULL,
  setpoint_percent INT NOT NULL,
  source ENUM('AC', 'DC', 'NO') DEFAULT 'DC',
  cut_off_voltage DECIMAL(10, 2) NOT NULL,
  cut_off_energy INT NOT NULL,
  timer_value VARCHAR(8) DEFAULT '00:00:00',
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT single_settings UNIQUE (id)
);

-- Sample readings data
INSERT INTO readings (voltage, current, power, energy, temperature, is_started, time_now)
VALUES 
  (123.1, 16.5, 511.2, 1651, 125, true, '00:05:12'),
  (122.9, 16.4, 510.0, 1700, 126, true, '00:08:45'),
  (122.7, 16.6, 513.5, 1750, 127, true, '00:12:30');
  
-- Insert initial settings if table is empty
INSERT INTO settings (id, setpoint, setpoint_percent, source, cut_off_voltage, cut_off_energy, timer_value)
SELECT 1, 500, 5, 'DC', 70.5, 2000, '00:00:00'
FROM dual
WHERE NOT EXISTS (SELECT 1 FROM settings);
