# Codes for Mathematical Analysis


We mathematically derive the ability of BalanceSketch to identify FlowBursts in Section 3.6. We derive the Markov process of random variable $X_i$ in Lemma 3.1, and the ability of BalanceSketch to detect FlowBurst in Theorem 3.1. Although we cannot obtain the analytical solution of $P_f$ from Lemma 3.1, we can mathematically calculate the numerical solution of $P_f$ under a specific setting. Here, we present the related codes for numerical simulation. 


## File Description 

- `math.m`:  The `matlab` codes for calculating $P_f$ according to the mathematical model in Section 3.6. We first fix $n_1$, $n_2$, and $\lambda$ to see how $P_f$ changes with $\mathcal{F}$, and save the results in `F.csv`. Then we fix $n_2$ and $\mathcal{F}$ to see how $P_f$ changes with $n_1$ and $\lambda$, and save the results in `n1&lambda.csv`. 


- `F.csv`: Results of $P_f$ under different $\mathcal{F}$. 

- `n1&lambda.csv`: Results of $P_f$ under different $n_1$ and $\lambda$. 


.

