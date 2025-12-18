/** @type {import('tailwindcss').Config} */
export default {
    content: [
        "./index.html",
        "./src/**/*.{js,ts,jsx,tsx}",
    ],
    theme: {
        extend: {
            colors: {
                'aether-black': '#0a0a0a', // Void Black
                'aether-red': '#f43f5e',   // Rose 500
                'aether-dark': '#171717',  // Neutral 900
            }
        },
    },
    plugins: [],
}
