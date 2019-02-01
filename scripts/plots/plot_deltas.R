#devtools::install_github("slowkow/ggrepel")

(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))

dd <- fread("output.csv")

dd %>%  group_by(algorithm, query_selectivity, query_pattern, delta, repetition) %>%  mutate(query = 1:n(), cumtime=cumsum(query_time)) %>% as.data.frame() -> ddq

ddq %>% select(algorithm, query, query_selectivity, query_pattern, delta, query_time, cumtime, converged) %>% group_by(algorithm, query, query_selectivity, query_pattern, delta) %>% summarise_all(funs(median)) -> aggr


QUERY_PATTERN_PLOT <- 4
QUERY_SELECTIVITY_PLOT <- .01

ORDERING = c("ppbeh"=1, 'ppbew'=2, 'ppcrk'=3, 'pprs'=4)
cols <- c("P. Cracking"='#493829',"P. Bucket"='#668d3c',"P. Radix (LSD)"='#cc0000', "P. Radix (MSD)"='#855723')

aggr %>% filter(query==1, query_selectivity==QUERY_SELECTIVITY_PLOT, query_pattern==QUERY_PATTERN_PLOT) %>% filter(algorithm %in% c("ppms", "ppcrk", "ppbew", "ppbeh", "pprs")) -> pdata

pdata %>% mutate(label=recode(algorithm, ppbeh="P. Bucket", ppcrk="P. Cracking", pprs="P. Radix (LSD)",ppbew = "P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm]) -> pdata2



pdf("delta_first_query.pdf", width=6, height=5)
ggplot(pdata2, aes(y=query_time, x=delta, color=label, group=label, shape=label)) + geom_line(size=1.5) + geom_point(size=3) +scale_x_log10()  + theme + ylab("Query Time (s)") + xlab(expression(delta))  +theme(legend.text = element_text(size=28), legend.justification = c(0, 1), legend.position = c(0, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()


aggr %>% filter(algorithm=="fs", query_selectivity==QUERY_SELECTIVITY_PLOT, query_pattern==QUERY_PATTERN_PLOT) -> fstiming

aggr %>% filter( query_selectivity==QUERY_SELECTIVITY_PLOT, query_pattern==QUERY_PATTERN_PLOT) %>% filter(algorithm %in% c("ppms", "ppcrk", "ppbew", "ppbeh", "pprs")) -> progressive

progressive %>% left_join(fstiming, by="query") %>% filter(cumtime.x < cumtime.y) %>% group_by(algorithm.x, delta.x) %>% mutate(min_q = min(query)) %>% filter(query == min_q) %>% mutate(label=recode(algorithm.x, ppbeh="P. Bucket", ppcrk="P. Cracking", pprs="P. Radix (LSD)",ppbew = "P. Radix (MSD)")) %>% arrange(ORDERING[algorithm.x]) -> pdata2



pdf("delta_break_even.pdf", width=6, height=5)

ggplot(pdata2, aes(y=min_q, x=delta.x, color=label, group=label, shape=label)) + geom_line(size=1.5) + geom_point(size=3) + theme + ylab("Query Number (#)") + xlab(expression(delta)) + scale_x_log10()  + theme(legend.text = element_text(size=28),legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()




progressive %>% group_by(algorithm, delta) %>% summarise(sum_time = sum(query_time)) %>% mutate(label=recode(algorithm, ppbeh="P. Bucket", ppcrk="P. Cracking", pprs="P. Radix (LSD)",ppbew = "P. Radix (MSD)" )) %>% arrange(ORDERING[algorithm]) -> pdata2

pdf("delta_total_time.pdf", width=6, height=5)
ggplot(pdata2, aes(y=sum_time, x=delta, color=label, group=label, shape=label)) + geom_line(size=1.5) + geom_point(size=3) + theme + ylab("Total Time (s)") + xlab(expression(delta)) + scale_x_log10()  +scale_y_continuous(limits=c(0, max(pdata2$sum_time)))  + theme(legend.text = element_text(size=28),legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols) 
dev.off()


progressive %>% group_by(algorithm, delta) %>% summarise(convergence_nr = max(converged)) %>% mutate(label=recode(algorithm,ppbeh="P. Bucket", ppcrk="P. Cracking", pprs="P. Radix (LSD)",ppbew = "P. Radix (MSD)")) %>% arrange(ORDERING[algorithm]) -> pdata2
pdata2 %>% filter(convergence_nr > 0) -> pdata3

pdf("delta_convergence.pdf", width=6, height=5)
ggplot(pdata3, aes(y=convergence_nr, x=delta, color=label, group=label, shape=label)) + geom_line(size=1.5) + geom_point(size=3) + theme + ylab("Query Number (#)") + xlab(expression(delta))  +scale_x_log10()  + theme(legend.text = element_text(size=28),legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank())  + theme(legend.key.size=unit(1,"cm")) + scale_color_manual(values=cols)
dev.off()





