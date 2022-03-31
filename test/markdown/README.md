Test+ Test.Initial+ 
Test.Initial- Test.Prepare+ 
Test.Prepare- Test.TestSteps+ Test.TestSteps.Initial+ 
Test.TestSteps.Initial- Test.TestSteps.Step1+ 
Test.TestSteps.Step1- Test.TestSteps.Step2+ 
Test.TestSteps.Step2- Test.TestSteps.Step3+ 
Test.Initial- Test.Prepare+ 
Test.Prepare- 
State Coverage: 0.857143 ( 85.7143% )
Transition Coverage: 1 ( 100% )


<table>
 <tr> <th>abc</th> <th> def</th> </tr>
 <tr> <td> 1 </td>  <td> 2 </td> </tr>
</table>

__State Machine__  *__Test__ * :

__States__ :

__Initial__ 

   ░░░░░   __Prepare__ 



__Transitions__ :

Initial -`➜`Prepare



Prepare -`➜`TestSteps



__State Machine__  *__TestSteps__ * :

__States__ :

__Initial__ 

   ░░░      __Step1__ 

   ░░░      __Step2__ 

   ░░░      __Step3__ 

__Step4__ 



__Transitions__ :

Initial -`➜`Step1



Step1   -`➜`Step2



Step2   -`➜`Step3






# [Simulation] 


## Steps


```javascript
Start state machine Test.
```
