require("dotenv").config();

const express = require("express");
const cors = require("cors");
const connectDB = require("./config/db");

const app = express();

app.use(cors());
app.use(express.json());

connectDB();

// MQTT
require("./services/mqttService");

// Routes
app.use("/api/auth", require("./routes/authRoutes"));
app.use("/api", require("./routes/alertRoutes"));

app.get("/", (req, res) => {
  res.send("🚀 E.C.H.O Backend Running");
});

const PORT = process.env.PORT || 5000;

app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});