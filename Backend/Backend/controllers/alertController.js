const Alert = require("../models/Alert");
const User = require("../models/User");
const sendAlertEmail = require("../services/emailService");

exports.receiveAlert = async (req, res) => {
  const { nodeId, type, confidence, lat, lng } = req.body;

  const alert = new Alert({ nodeId, type, confidence, lat, lng });
  await alert.save();

  if (type === "Drone" && confidence > 0.8) {
    const users = await User.find();
    const emails = users.map(u => u.email);

    await sendAlertEmail({ nodeId, type, confidence, emails });
  }

  res.json({ message: "Alert received" });
};

exports.getAlerts = async (req, res) => {
  const alerts = await Alert.find().sort({ timestamp: -1 });
  res.json(alerts);
};

exports.getStats = async (req, res) => {
  const total = await Alert.countDocuments();
  const drones = await Alert.countDocuments({ type: "Drone" });

  res.json({ total, drones });
};