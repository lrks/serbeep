#png("plot6.png")
#
#tmp02 <- read.table("result1-02.dat")
#x02 <- cbind(tmp02[1:1], tmp02[2:2] - tmp02[1, 2])
#
#tmp03 <- read.table("result1-03.dat")
#x03 <- cbind(tmp03[1:1], tmp03[2:2] - tmp03[1, 2])
#
#tmp04 <- read.table("result1-04.dat")
#x04 <- cbind(tmp04[1:1], tmp04[2:2] - tmp04[1, 2])
#
#x <- cbind(x02[1:1], (x02 - x02)[2:2], (x03 - x02)[2:2], (x04 - x02)[2:2])
#
#plot(cbind(x[1:1], x[4:4]), type="l", col="green")
#lines(cbind(x[1:1], x[2:2]), type="l", col="red")
#lines(cbind(x[1:1], x[3:3]), type="l", col="blue")
#graphics.off()
#

png("sleep.png")
x1 <- read.table('noload-sleep.dat')
x2 <- read.table('load-sleep.dat')
plot(x1, ylim=c(0, 0.004), col="red")
par(new=T)
plot(x2,ylim=c(0, 0.004), col="blue")
graphics.off()

x1 <- read.csv('syscall-noload.dat', header=T) * 1000 * 1000
x2 <- read.csv('syscall-load.dat', header=T) * 1000 * 1000

s = min(x1, x2)
e = max(x1, x2)

png('test.png')
stripchart(x1, pch=1, col="red", ylim=c(s, e), vert=T, ylab="micro sec")
par(new=T)
stripchart(x2, pch=1, col="blue", ylim=c(s, e), vert=T, ylab="micro sec")
legend("topleft", legend=c("noload", "load"), col=c("red", "blue"), pch = 1)
title("strace -T")
graphics.off()



x1 <- read.table('noload-tboff.dat')
x2 <- read.table('load-tboff.dat')

s = min(x1[2:2], x2[2:2])
e = max(x1[2:2], x2[2:2])

png('majika.png')
plot(x1, col="red", ylim=c(s, e), xlab="Line number", ylab="sec")
par(new=T)
plot(x2, col="blue", ylim=c(s, e), xlab="Line number", ylab="sec")
legend("topleft", legend=c("noload", "load"), col=c("red", "blue"), pch = 1)
title("exec -> ioctl/nanosleep (TurboBoost off)")
graphics.off()

x1 <- read.table('../original.dat')
x2 <- read.table('serbeep-load-02.dat')
x3 <- read.table('serbeep-load-03.dat')
x4 <- read.table('serbeep-load-04.dat')

x2 <- cbind(x1[1:1], (x2 - x1)[2:2])
x3 <- cbind(x1[1:1], (x3 - x1)[2:2])
x4 <- cbind(x1[1:1], (x4 - x1)[2:2])

xrange = c(min(x1[1:1]), max(x1[1:1]))
yrange = c(min(x2[2:2], x3[2:2], x4[2:2]), max(x2[2:2], x3[2:2], x4[2:2]))

png('serbeep.png')
plot(x2, col="red", xlim=xrange, ylim=yrange, xlab="", ylab="")
par(new=T)
plot(x3, col="blue", xlim=xrange, ylim=yrange, xlab="", ylab="")
par(new=T)
plot(x4, col="green", xlim=xrange, ylim=yrange, xlab="Note", ylab="ms")

legend("topright", legend=c("Server A", "Server B", "Server C"), col=c("red", "blue", "green"), pch = 1)
title("serbeep(load)")
graphics.off()

