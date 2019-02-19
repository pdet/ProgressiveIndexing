(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("ggplot2", "ggthemes"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))


n <- 10000

random <- head(read.csv("random.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
sequential <- head(read.csv("sequential.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
sequential_random <- head(read.csv("sequential_random.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
zoom_in <- head(read.csv("zoom_in.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
seq_zoom_in <- head(read.csv("seq_zoom_in.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
skew <- head(read.csv("skew.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
zoom_out_alt <- head(read.csv("zoom_out_alt.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
periodic <- head(read.csv("periodic.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)
zoom_in_alt <- head(read.csv("zoom_in_alt.csv", header=F, sep=";", col.names=c("query", "start", "stop")), n)


pdf("random.pdf", width=8, height=5)
ggplot(random, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()




pdf("sequential.pdf", width=8, height=5)
ggplot(sequential, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()


pdf("sequential_random.pdf", width=8, height=5)
ggplot(sequential_random, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("zoom_in.pdf", width=8, height=5)
ggplot(zoom_in, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("seq_zoom_in.pdf", width=8, height=5)
ggplot(seq_zoom_in, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("skew.pdf", width=8, height=5)
ggplot(skew, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("zoom_out_alt.pdf", width=8, height=5)
ggplot(zoom_out_alt, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("periodic.pdf", width=8, height=5)
ggplot(periodic, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()

pdf("zoom_in_alt.pdf", width=8, height=5)
ggplot(zoom_in_alt, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^9), labels=c("0", expression(10^9))) + xlab("Query (#)") + ylab("Query Range")
dev.off()