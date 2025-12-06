// Configuration for API and Socket.io URLs
// Sử dụng environment variables hoặc fallback về localhost

// Lấy API base URL từ environment variable hoặc dùng default
// Trong production, Vite sẽ thay thế import.meta.env.VITE_API_URL khi build
const getApiBaseUrl = () => {
  // Nếu có VITE_API_URL trong env, dùng nó
  if (import.meta.env.VITE_API_URL) {
    return import.meta.env.VITE_API_URL;
  }
  
  return 'http://localhost:3000';
};

// Lấy Socket.io URL từ environment variable hoặc dùng default
const getSocketUrl = () => {
  // Nếu có VITE_SOCKET_URL trong env, dùng nó
  if (import.meta.env.VITE_SOCKET_URL) {
    return import.meta.env.VITE_SOCKET_URL;
  }
  
  // Production fallback
  return 'http://localhost:3000';
};

export const config = {
  apiBaseUrl: getApiBaseUrl(),
  socketUrl: getSocketUrl(),
};

// Log config trong development để debug
if (import.meta.env.NODE_ENV === "development") {
  console.log('Frontend Config:', config);
}
