png("plot6.png")

tmp02 <- read.table("result1-02.dat")
x02 <- cbind(tmp02[1:1], tmp02[2:2] - tmp02[1, 2])

tmp03 <- read.table("result1-03.dat")
x03 <- cbind(tmp03[1:1], tmp03[2:2] - tmp03[1, 2])

tmp04 <- read.table("result1-04.dat")
x04 <- cbind(tmp04[1:1], tmp04[2:2] - tmp04[1, 2])

x <- cbind(x02[1:1], (x02 - x02)[2:2], (x03 - x02)[2:2], (x04 - x02)[2:2])

plot(cbind(x[1:1], x[4:4]), type="l", col="green")
lines(cbind(x[1:1], x[2:2]), type="l", col="red")
lines(cbind(x[1:1], x[3:3]), type="l", col="blue")
graphics.off()

