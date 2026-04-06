const express = require("express");
const router = express.Router();

const {
  receiveAlert,
  getAlerts,
  getStats
} = require("../controllers/alertController");

const { verifyToken } = require("../middleware/authMiddleware");

router.post("/alert", receiveAlert); // ESP8266
router.get("/alerts", verifyToken, getAlerts);
router.get("/stats", verifyToken, getStats);

module.exports = router;