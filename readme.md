## A TCP chatroom and an UDP chatroom
## Prof. HaimingJin

## Assuming Invironment
* $h1,h2,h3,h4$ have IP addr $10.0.0.\{1,2,3,4\}$ respectly.
* UDP send from 10.0.0.i (filter base on ip)
* **NO** space in one message because I use cin>>

## Implement Method
`poll()`. I doesn't set port reuse. 

