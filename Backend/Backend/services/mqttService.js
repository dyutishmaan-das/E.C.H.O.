const mqtt = require("mqtt");
const Alert = require("../models/Alert");
const sendAlertEmail = require("./emailService");
const User = require("../models/User");

const client = mqtt.connect(process.env.MQTT_BROKER);

client.on("connect", () => {
  console.log("MQTT Connected");
  client.subscribe("echo/uav");
});

client.on("message", async (topic, message) => {
  try {
    const data = JSON.parse(message.toString());

    const alert = new Alert(data);
    await alert.save();

    if (data.type === "Drone" && data.confidence > 0.8) {
      const users = await User.find();
      const emails = users.map(u => u.email);

      await sendAlertEmail({ ...data, emails });
    }

  } catch (err) {
    console.error(err);
  }
});

module.exports = client;