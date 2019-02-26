(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("ggplot2", "ggthemes"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))


n <- 150
for (i in 1:16){
aux <- head(read.csv(paste(as.character(i),".csv",sep=""), header=F, sep=";", col.names=c("query", "start", "stop")), n)
pdf(paste(as.character(i),".pdf",sep=""), width=8, height=5)
ggplot(aux, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^8), labels=c("0", expression(10^8))) + xlab("Query (#)") + ylab("Query Range")
dev.off()
}

# pdf("seq.pdf", width=8, height=5)
# ggplot(seq, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^8), labels=c("0", expression(10^8))) + xlab("Query (#)") + ylab("Query Range")
# dev.off()




# pdf("skew.pdf", width=8, height=5)
# ggplot(skew, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^8), labels=c("0", expression(10^8))) + xlab("Query (#)") + ylab("Query Range")
# dev.off()



# pdf("rnd.pdf", width=8, height=5)
# ggplot(rnd, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^8), labels=c("0", expression(10^8))) + xlab("Query (#)") + ylab("Query Range")
# dev.off()

# pdf("mxd.pdf", width=8, height=5)
# ggplot(mxd, aes(query)) + geom_segment(aes(y=start, yend=stop, x=query,xend=query), size=1.2) + theme + scale_y_continuous(breaks=c(0, 10^8), labels=c("0", expression(10^8))) + xlab("Query (#)") + ylab("Query Range")
# dev.off()