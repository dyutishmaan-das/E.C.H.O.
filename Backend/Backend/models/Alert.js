const mongoose = require("mongoose");

const alertSchema = new mongoose.Schema({
  nodeId: String,
  type: String,
  confidence: Number,
  lat: Number,
  lng: Number,
  timestamp: { type: Date, default: Date.now }
});

module.exports = mongoose.model("Alert", alertSchema);