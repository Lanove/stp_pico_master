const { pool } = require('../config/db');

class ReadingsModel {
  // Get latest readings
  async getLatestReadings() {
    try {
      const [rows] = await pool.query(`
        SELECT * FROM readings 
        ORDER BY timestamp DESC 
        LIMIT 1
      `);
      return rows[0] || null;
    } catch (error) {
      console.error('Error fetching latest readings:', error);
      throw error;
    }
  }

  // Get readings history (most recent first)
  async getReadingsHistory(limit = 100) {
    try {
      const [rows] = await pool.query(`
        SELECT * FROM readings 
        ORDER BY timestamp DESC 
        LIMIT ?
      `, [limit]);
      return rows;
    } catch (error) {
      console.error('Error fetching readings history:', error);
      throw error;
    }
  }

  // Get readings history within a timespan (for plotting)
  async getReadingsHistoryByTimespan(timespanSeconds = 300) {
    try {
      const [rows] = await pool.query(`
        SELECT * FROM readings 
        WHERE timestamp >= DATE_SUB(NOW(), INTERVAL ? SECOND)
        ORDER BY timestamp ASC
      `, [timespanSeconds]);
      return rows;
    } catch (error) {
      console.error('Error fetching readings history by timespan:', error);
      throw error;
    }
  }

  // Insert new readings
  async insertReading(reading) {
    try {
      const [result] = await pool.query(`
        INSERT INTO readings 
        (voltage, current, power, energy, temperature, is_started, time_now, timestamp) 
        VALUES (?, ?, ?, ?, ?, ?, ?, NOW())
      `, [
        reading.voltage, 
        reading.current, 
        reading.power, 
        reading.energy,
        reading.temperature || 30, // Default to 30°C
        reading.is_started !== undefined ? reading.is_started : false,
        reading.time_now || '00:00:00' // Default to 00:00:00 if not provided
      ]);
      return result.insertId;
    } catch (error) {
      console.error('Error inserting reading:', error);
      throw error;
    }
  }

  // Get settings
  async getSettings() {
    try {
      const [rows] = await pool.query(`
        SELECT * FROM settings 
        ORDER BY id DESC 
        LIMIT 1
      `);
      return rows[0] || null;
    } catch (error) {
      console.error('Error fetching settings:', error);
      throw error;
    }
  }

  // Update settings
  async updateSettings(settings) {
    try {
      // Log the incoming settings data for debugging
      console.log('Updating settings with data:', JSON.stringify(settings));
      
      // Validate source value
      if (settings.source && !['AC', 'DC', 'NO'].includes(settings.source)) {
        console.error('Invalid source value:', settings.source);
        throw new Error(`Invalid source value: ${settings.source}. Must be AC, DC, or NO.`);
      }
      
      // First check if settings entry exists to determine whether to insert or update
      const [existing] = await pool.query('SELECT id FROM settings LIMIT 1');
      let result;
      
      if (existing.length > 0) {
        // If entry exists, perform an UPDATE
        console.log('Existing settings found. Performing UPDATE...');
        const query = `
          UPDATE settings 
          SET 
            setpoint = ?,
            setpoint_percent = ?,
            source = ?,
            cut_off_voltage = ?,
            cut_off_energy = ?,
            timer_value = ?
          WHERE id = ?
        `;
        
        [result] = await pool.query(query, [
          settings.setpoint || 500,
          settings.setpoint_percent || 5,
          settings.source || 'DC',
          settings.cut_off_voltage || 70.5,
          settings.cut_off_energy || 2000,
          settings.timer_value || '00:00:00',
          existing[0].id
        ]);
      } else {
        // If no entry exists, perform an INSERT
        console.log('No existing settings found. Performing INSERT...');
        const query = `
          INSERT INTO settings 
          (setpoint, setpoint_percent, source, cut_off_voltage, cut_off_energy, timer_value) 
          VALUES (?, ?, ?, ?, ?, ?)
        `;
        
        [result] = await pool.query(query, [
          settings.setpoint || 500,
          settings.setpoint_percent || 5,
          settings.source || 'DC',
          settings.cut_off_voltage || 70.5,
          settings.cut_off_energy || 2000,
          settings.timer_value || '00:00:00'
        ]);
      }
      
      console.log('Settings update SQL result:', result);
      return result;
    } catch (error) {
      console.error('Error updating settings:', error);
      throw error;
    }
  }
}

module.exports = new ReadingsModel();