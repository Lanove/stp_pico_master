const express = require('express');
const router = express.Router();
const ReadingsController = require('../controllers/readings.controller');

// GET all readings (for testing)
router.get('/', ReadingsController.getReadingsHistory);

// GET latest readings
router.get('/latest', ReadingsController.getLatestReadings);

// GET readings history
router.get('/history', ReadingsController.getReadingsHistory);

// GET readings history by timespan
router.get('/history/timespan', ReadingsController.getReadingsHistoryByTimespan);

// POST new reading
router.post('/', ReadingsController.createReading);

// GET settings
router.get('/settings', ReadingsController.getSettings);

// PUT settings
router.put('/settings', ReadingsController.updateSettings);

module.exports = router;