// Load environment variables first
require('dotenv').config();

const express = require('express');
const cors = require('cors');
const { testConnection } = require('./config/db');
const readingsRoutes = require('./routes/readings.routes');

// Create Express app
const app = express();
const PORT = process.env.PORT || 5000;

// Middleware
app.use(cors());
app.use(express.json());

// Test database connection
testConnection();

// Routes
app.use('/api/readings', readingsRoutes);

// Base route
app.get('/', (req, res) => {
  res.json({ message: 'Loadbank Dashboard API is running' });
});

// Start server - bind to all interfaces for network access
app.listen(PORT, '0.0.0.0', () => {
  console.log(`Server running on port ${PORT}`);
  console.log(`Local access: http://localhost:${PORT}`);
  console.log(`Network access: http://192.168.1.22:${PORT}`);
  console.log(`mDNS access: http://kohigashi.local:${PORT}`);
  console.log(`Database connection successful`);
});