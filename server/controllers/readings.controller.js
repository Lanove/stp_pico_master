const ReadingsModel = require('../models/readings.model');

class ReadingsController {
  // Get latest readings
  async getLatestReadings(req, res) {
    try {
      const readings = await ReadingsModel.getLatestReadings();
      if (!readings) {
        console.log('No readings found in database');
        return res.status(404).json({ message: 'No readings found' });
      }
      console.log('Successfully retrieved latest readings:', readings);
      res.status(200).json(readings);
    } catch (error) {
      console.error('Error in getLatestReadings controller:', error);
      res.status(500).json({ message: 'Internal server error' });
    }
  }

  // Get readings history
  async getReadingsHistory(req, res) {
    try {
      const limit = req.query.limit ? parseInt(req.query.limit) : 100;
      const readings = await ReadingsModel.getReadingsHistory(limit);
      res.status(200).json(readings);
    } catch (error) {
      console.error('Error in getReadingsHistory controller:', error);
      res.status(500).json({ message: 'Internal server error' });
    }
  }

  // Get readings history by timespan
  async getReadingsHistoryByTimespan(req, res) {
    try {
      const timespanParam = req.query.timespan || '5m';
      
      // Convert timespan string to seconds
      const convertToSeconds = (timespan) => {
        const match = timespan.match(/^(\d+)([smhd])$/);
        if (!match) return 300; // default 5 minutes
        
        const value = parseInt(match[1]);
        const unit = match[2];
        
        switch (unit) {
          case 's': return value;
          case 'm': return value * 60;
          case 'h': return value * 3600;
          case 'd': return value * 86400;
          default: return 300;
        }
      };
      
      const timespanSeconds = convertToSeconds(timespanParam);
      console.log(`🎯 Timespan API called with timespan: ${timespanParam} (${timespanSeconds} seconds)`);
      const readings = await ReadingsModel.getReadingsHistoryByTimespan(timespanSeconds);
      console.log(`📊 Retrieved ${readings.length} readings for timespan`);
      res.status(200).json(readings);
    } catch (error) {
      console.error('Error in getReadingsHistoryByTimespan controller:', error);
      res.status(500).json({ message: 'Internal server error' });
    }
  }

  // Create new reading
  async createReading(req, res) {
    try {
      const { voltage, current, power, energy, temperature, source, is_started, time_now } = req.body;
      
      // Basic validation
      if (voltage === undefined || current === undefined || power === undefined || energy === undefined) {
        return res.status(400).json({ message: 'Missing required fields' });
      }
      
      // Set default values for optional fields (excluding source since it's not in DB)
      const readingData = {
        voltage,
        current,
        power,
        energy,
        temperature: temperature !== undefined ? temperature : 30, // Default to 30°C if not provided
        is_started: is_started !== undefined ? is_started : false,
        time_now: time_now || null // Let server derive timestamp if not provided
      };
      
      // Log the received data including source for debugging, but don't store it
      console.log('Received sensor data from Pico:', {
        ...readingData,
        source: source || 'DC' // Log source but don't store in DB
      });
      
      const result = await ReadingsModel.insertReading(readingData);
      res.status(201).json({ 
        message: 'Reading created successfully', 
        id: result 
      });
    } catch (error) {
      console.error('Error in createReading controller:', error);
      res.status(500).json({ message: 'Internal server error' });
    }
  }

  // Get settings
  async getSettings(req, res) {
    try {
      const settings = await ReadingsModel.getSettings();
      if (!settings) {
        console.log('No settings found in database');
        return res.status(404).json({ message: 'No settings found' });
      }
      console.log('Successfully retrieved settings:', settings);
      res.status(200).json(settings);
    } catch (error) {
      console.error('Error in getSettings controller:', error);
      res.status(500).json({ message: 'Internal server error' });
    }
  }

  // Update settings
  async updateSettings(req, res) {
    try {
      console.log('Received settings update request:', req.body);
      
      const { 
        setpoint, 
        setpoint_percent, 
        setpointPercent,
        source, 
        cut_off_voltage,
        cutOffVoltage, 
        cut_off_energy,
        cutOffEnergy, 
        timer_value,
        timerValue, 
        is_started,
        isStarted 
      } = req.body;
      
      // Support both snake_case and camelCase naming
      const settingsData = {
        setpoint: setpoint,
        setpoint_percent: setpoint_percent || setpointPercent,
        source: source,
        cut_off_voltage: cut_off_voltage || cutOffVoltage,
        cut_off_energy: cut_off_energy || cutOffEnergy,
        timer_value: timer_value || timerValue,
        is_started: is_started !== undefined ? is_started : isStarted
      };
      
      console.log('Processed settings data:', settingsData);
      
      // Basic validation
      if (settingsData.setpoint === undefined) {
        return res.status(400).json({ message: 'Missing required field: setpoint' });
      }
      
      if (settingsData.source === undefined || !['AC', 'DC', 'NO'].includes(settingsData.source)) {
        return res.status(400).json({ message: 'Invalid or missing source value. Must be AC, DC, or NO' });
      }
      
      const result = await ReadingsModel.updateSettings(settingsData);
      console.log('Settings update successful with result:', result);
      res.status(200).json({ message: 'Settings updated successfully', data: settingsData });
    } catch (error) {
      console.error('Error in updateSettings controller:', error);
      res.status(500).json({ message: 'Internal server error', error: error.message });
    }
  }
}

module.exports = new ReadingsController();