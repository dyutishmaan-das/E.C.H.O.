const express = require("express");
const router = express.Router();
const { registerUser, login } = require("../controllers/authController");
const { verifyToken, isAdmin } = require("../middleware/authMiddleware");

// ✅ Test route (IMPORTANT)
router.get("/", (req, res) => {
  res.send("Auth API working ✅");
});

// 🔐 Auth routes
router.post("/login", login);
router.post("/register", verifyToken, isAdmin, registerUser);

module.exports = router;