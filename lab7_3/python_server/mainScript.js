const date = new Date();

const weekdays = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday']
const monthNames = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']

const currentDayOfMonth = date.getDate();
const currentMonth = date.getMonth();
const currentYear = date.getFullYear();
const currentDayOfWeek = date.getDay();

const currentDate = `${weekdays[currentDayOfWeek]}, ${monthNames[currentMonth]} ${currentDayOfMonth} ${currentYear}`

document.getElementById('dateToday').innerHTML = `Today is ${currentDate}`;
