import { spawn } from 'node:child_process'

const children = [
  spawn('node', ['server/index.js'], { stdio: 'inherit' }),
  spawn('vite', ['--host', '127.0.0.1'], { stdio: 'inherit' }),
]

let shuttingDown = false

function shutdown(code = 0) {
  if (shuttingDown) return
  shuttingDown = true
  children.forEach((child) => {
    if (!child.killed) child.kill('SIGTERM')
  })
  setTimeout(() => process.exit(code), 150)
}

children.forEach((child) => {
  child.on('exit', (code) => {
    if (!shuttingDown && code !== 0) shutdown(code || 1)
  })
})

process.on('SIGINT', () => shutdown(0))
process.on('SIGTERM', () => shutdown(0))
