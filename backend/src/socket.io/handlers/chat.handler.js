const Message = require("../../models/message.model");

module.exports =  (io, socket) => {
  socket.on("send-message", async (data, updateStatus) => {
    try {
      const savedMessage = await Message.create({
        senderId: socket.user._id,
        receiverId: data.receiverId,
        content: data.content
      });
     
      console.log(savedMessage.createdAt)
      updateStatus({success: true});
      io.to(data.receiverId.toString()).emit("receive-message", {
        content: data.content,
        sentAt: savedMessage.createdAt
      });

    } catch (error) {
      console.log(error);
      updateStatus({success: false})
    }
    
  })
  // Xử lý khi user bắt đầu gõ
  socket.on("typing-start", (data) => {
    const receiverId = data.receiverId;
    if(!receiverId) return;

    // Gửi thông báo tới người nhận rằng user này đang gõ
    io.to(receiverId.toString()).emit("user-typing", {
      userId: socket.user._id.toString(),
      isTyping: true
    })
  })
  
  // Xử lý khi user dừng gõ
  socket.on("typing-stop", (data) => {
    const receiverId = data.receiverId;
    if(!receiverId) return;

    // Gửi thông báo tới người nhận rằng user này đã dừng gõ
    io.to(receiverId.toString()).emit("user-typing", {
      userId: socket.user._id.toString(),
      isTyping: false
    })
  })
} 