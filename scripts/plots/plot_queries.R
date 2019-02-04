#devtools::install_github("slowkow/ggrepel")

(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer"))

theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))

dd <- fread("output.csv")
dd %>%  group_by(algorithm, query_pattern, delta) %>%  mutate(query = 1:n(), cumtime=cumsum(query_time)) %>% as.data.frame() -> ddq

ddq %>% select(algorithm, query, query_pattern, delta, query_time, cumtime) %>% group_by(algorithm, query, query_pattern, delta) -> aggr

ORDERING = c('fs' = 1,'std' = 2, 'stc' = 3,'cgi'= 4, 'pstc'= 5, 'fi' = 6, 'pqs' = 7, 'pqscm' = 8)
cols <- c("P. Stc 10%[10]"='#bdd09f',"P. Quick"='#493829',"Cracking"='#8f3b1b',"P. Quick [Cost Model]"='#4e6172',"Stochastic"='#d57500',"Coarse"='#404f24',"Index"='#b99c6b', "Scan"='#a9a18c')

aggr %>% filter(query==1) -> pdata

pdata %>% mutate(label=recode(algorithm, pqs="P. Quick", pqscm="P. Quick [Cost Model]", cgi="Coarse", std="Cracking", fs="Scan", stc="Stochastic", fi="Index", pstc="P. Stc 10%[10]")) -> pdata2

pdata2$label <- factor(pdata2$label)

pdata3 <- filter(pdata2, !(algorithm %in% c("stc"))) %>% arrange(ORDERING[algorithm]) 

pdf("first_query.pdf", width=8, height=5)
ggplot(pdata3, aes(y=query_time, x=reorder(label, query_time), fill=label)) + geom_bar(stat="identity") + geom_text(aes(label=query_time), vjust=0) + theme + ylab("Query Time (s)") + theme(axis.text.x = element_text(angle = 90, vjust=.35)) + xlab("") + scale_fill_manual(values=cols)
dev.off()

aggr  -> pdata


pdata %>% mutate(label=recode(algorithm, pqs="P. Quick", pqscm="P. Quick [Cost Model]", cgi="Coarse", std="Cracking", fs="Scan", stc="Stochastic", fi="Index", pstc="P. Stc 10%[10]")) -> pdata2
pdata2 <- filter(pdata2, query < 1000)

# fitimes <- pdata2 %>% filter(query > 1) %>% group_by(algorithm)


# fitime <- fitimes
# names(fitime) <- fitimes$algorithm


pdata3 <- filter(pdata2, !(algorithm %in% c("fi", "fs"))) %>% arrange(ORDERING[algorithm]) 

pdata4 <- filter (pdata3, query == 999)
pdf("performance_time.pdf", width=8, height=5)
ggplot(pdata3, aes(y=query_time, x=query, color=reorder(label, ORDERING[algorithm]), group=reorder(label, ORDERING[algorithm]), shape=reorder(label, ORDERING[algorithm]))) + geom_line( size=1)+ geom_point(data=pdata4,size=3)  + theme + ylab("Query Time (log(s))") + xlab("Query (#)") + theme (legend.position = c(0,0)) + geom_hline(yintercept=0.1, color="black", linetype="dashed") + geom_hline(yintercept=0.000002, color="black", linetype="dashed") +  scale_y_log10(breaks=c(0.000002, 0.1, 1), labels=c("Index \n (0.000002)", "Scan\n (0.1)", "1.0"), limits=c(0.0000001, 10)) + guides(fill=guide_legend(nrow=3)) +theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)
dev.off()


pdata %>% mutate(label=recode(algorithm, pqs="P. Quick", pqscm="P. Quick [Cost Model]", cgi="Coarse", std="Cracking", fs="Scan", stc="Stochastic", fi="Index", pstc="P. Stc 10%[10]")) -> pdata2

pdata3 <- filter(pdata2, query < 1001) %>% arrange(ORDERING[algorithm]) 

pdata4 <- filter (pdata3, query == 999)
pdf("performance_time_cumulative.pdf", width=8, height=5)
ggplot(pdata3, aes(y=cumtime, x=query, color=reorder(label, ORDERING[algorithm]), group=reorder(label, ORDERING[algorithm]), shape=reorder(label, ORDERING[algorithm]))) + geom_line(size=1) + geom_point(data=pdata4,size=3) + theme + ylab("Cumulative Time (log(s))") + xlab("Query (#)") +scale_y_log10() + theme(legend.justification = c(0, 1), legend.position = c(0.65, 0.5), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols)

dev.off()
