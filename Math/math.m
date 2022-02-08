% P_f vs F
n_1=70;
n_2=30;
lambda=0.5;
res=zeros(20,1);
for F=0:19
    res(F+1,1)=flow(lambda,n_1,n_2,F);
end
writematrix([(0:19)',res],'F.csv');


% P_f vs lambda & N
n_2=30;
F=10;
res=zeros(10,51);
for lambda=1:10
    for n_1=50:100
        res(lambda,n_1-49)=flow(lambda/20,n_1,n_2,F);
    end
end
writematrix([0,50:100;((1:10)'/20),res],'n1&lambda.csv');


function res=flow(lambda,n_1,n_2,F)
% This function calculate the probability P_f based on the model in $3.6
P_X=zeros(n_1+1,n_1+n_2+1);
P_Y=zeros(n_1+1,1);
P_Y(1)=exp(-lambda);
for k=2:n_1+1
    P_Y(k)=P_Y(k-1)*lambda/(k-1);
end
for n=2:n_1+1
    for k=2:n
        P_X(k,n)=P_X(k-1:n-1,n-1)'*P_Y(1:n-k+1,1);
    end
    P_X(1,n)=1-sum(P_X(2:n,n));
end
res=0;
P_Y1=zeros(n_1+1,1);
P_Y1(1)=exp(-lambda*n_2);
for k=2:n_1+1
    P_Y1(k)=P_Y1(k-1)*lambda*n_2/(k-1);
end
for k=1:n_1-F
    res=res+sum(P_Y1(1:k,1))*P_X(F+k+1,n_1+1);
end
end
