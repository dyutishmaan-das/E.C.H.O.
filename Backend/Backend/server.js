require("dotenv").config();

const express = require("express");
const cors = require("cors");
const connectDB = require("./config/db");

// 🔥 Initialize App
const app = express();

// 🔹 Middleware
app.use(cors());
app.use(express.json());

// 🔹 Connect Database
connectDB();

// 🔹 Initialize MQTT Service
require("./services/mqttService");

// 🔹 Routes
app.use("/api", require("./routes/alertRoutes"));

// 🔹 Health Check Route
app.get("/", (req, res) => {
  res.status(200).json({
    status: "OK",
    message: "🚀 E.C.H.O Backend Running",
    time: new Date().toLocaleString()
  });
});

// 🔹 Email Test Route
const sendAlertEmail = require("./services/emailService");

app.get("/test-mail", async (req, res) => {
  try {
    await sendAlertEmail({
      nodeId: "TEST_NODE",
      type: "Drone",
      confidence: 0.95
    });

    res.status(200).json({
      success: true,
      message: "✅ Email Sent Successfully"
    });

  } catch (error) {
    console.error("❌ Email Error:", error.message);

    res.status(500).json({
      success: false,
      message: "❌ Failed to send email",
      error: error.message
    });
  }
});

// 🔹 Global Error Handler (Optional but useful)
app.use((err, req, res, next) => {
  console.error("🔥 Server Error:", err.stack);

  res.status(500).json({
    success: false,
    message: "Internal Server Error"
  });
});

// 🔹 Start Server
const PORT = process.env.PORT || 5000;

app.listen(PORT, () => {
  console.log(`🔥 Server running on port ${PORT}`);
});