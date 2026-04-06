const nodemailer = require("nodemailer");

const transporter = nodemailer.createTransport({
  service: "gmail",
  auth: {
    user: process.env.EMAIL_USER,
    pass: process.env.EMAIL_PASS
  }
});

const sendAlertEmail = async ({ nodeId, type, confidence, emails }) => {
  await transporter.sendMail({
    from: process.env.EMAIL_USER,
    to: emails,
    subject: "🚨 Drone Detected",
    html: `
      <h2>Drone Alert</h2>
      <p>Node: ${nodeId}</p>
      <p>Confidence: ${confidence}</p>
    `
  });
};

module.exports = sendAlertEmail;