#devtools::install_github("slowkow/ggrepel")

(function(lp) {
np <- lp[!(lp %in% installed.packages()[,"Package"])]
if(length(np)) install.packages(np,repos=c("http://cran.rstudio.com/"))
x <- lapply(lp,function(x){library(x,character.only=TRUE, quietly=TRUE, warn.conflicts=FALSE)}) 
})(c("dplyr", "ggplot2", "ggthemes", "ggrepel", "data.table", "RColorBrewer"))


theme <- theme_few(base_size = 24)  +  theme(legend.position = "none", plot.title = element_text(size=16, hjust=.5, family="mono"))

dd <- fread("output.csv")

dd %>%  group_by(algorithm, query_selectivity, query_pattern, delta, repetition, frequency,num_appends) %>%  mutate(query = 1:n(), cumtime=cumsum(query_time)) %>% as.data.frame() -> ddq

ddq %>% select(algorithm, query, query_selectivity, query_pattern, delta, query_time, cumtime, converged) %>% group_by(algorithm, query, query_selectivity, query_pattern, delta) %>% summarise_all(funs(median)) -> aggr


# QUERY_PATTERN_PLOT <- 1
# QUERY_SELECTIVITY_PLOT <- .1


ORDERING = c("crkmgi"=1, 'crkmri'=2, 'crkmci'=3, 'updprog'=4)
cols <- c("MGI"='#d57700',"MRI"='#bdd09f',"MCI"='#8f3b1b',"P. Mergesort"='#293829')



# aggr %>% filter(query_selectivity==QUERY_SELECTIVITY_PLOT, query_pattern==QUERY_PATTERN_PLOT) -> pdata

aggr %>% mutate(label=recode(algorithm, crkmgi="MGI", crkmri="MRI", crkmci="MCI", updprog="P. Mergesort")) -> pdata2


fitimes <- pdata2 %>% filter(query > 0) %>% group_by(algorithm) %>% summarise(m=median(query_time))

fitime <- fitimes$m
names(fitime) <- fitimes$algorithm


pdata3 <- filter(pdata2, query < 10000) %>% arrange(ORDERING[algorithm]) 

pdata3 %>% mutate(isadaptive=ifelse(algorithm %in% c("pstc", "std", "stc", "cgi","ppqs"), " ", " ")) -> pdata4
pdata5 <- filter (pdata3, query == 9000)


pdf("updates_time.pdf", width=8, height=5)
ggplot(pdata4, aes(y=query_time, x=query, color=label, group=label, shape=label)) + geom_line(size=1) + geom_point(data=pdata5,size=4)+ theme + ylab("Query Time (log(s))") + xlab("Query (#)") + scale_y_log10() + theme(legend.justification = c(1, 1), legend.position = c(1, 1), legend.background = element_rect(fill = "transparent", colour = "transparent"), legend.title=element_blank()) + scale_color_manual(values=cols) + facet_grid(~isadaptive)
dev.off()

