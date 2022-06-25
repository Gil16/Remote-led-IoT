# Remote-led-IoT

In this project we want to turn on/off the led using ESP32, AWS services and wix.

## ESP32 - Getting Started
First, we need to setup the local environment to connect to ESP32. 
In order to do so, we can use the tutorial below on how to install Arduino IDE and connect our ESP32 to it:
https://docs.google.com/document/d/1VpwGfn88FCGll56WMATK2d8IPXryUlR_CHACYzYPHW0/edit

Second, we need to create AWS account (this will require adding your credit card, but in our case we are using the free tier).
The AWS services we will use are: IoT Core, API Gateway, Lambda, IAM, S3

IMPORTANT Note!
Make sure your region is the same in each aws service.


## Creating a Thing:
Policy is needed for a thing in order to determine which actions it can do,
Go to AWS IoT Core service -> Security -> Policies -> Create
Name it "ESP32_POLICY" and paste the code below
```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "*"
    }
  ]
}
```

Now, navigate to: All devices -> Things -> Create: name it ESP32_LED, under Devuce Shadow choose: Unnamed shadow (classic), next save the certificates: aws root certificate, device certitiface and private key. (You will need them for secrets.h)
Attach the policy we created earlier aka "ESP32_POLICY" and we are done creating the thing.


## Certificates
Copy the code from main.ino , secrets.h
In secrets.h, replace the ****** with the certificates you saved before.
In order to connect your ESP to wifi, insert the wifi name/password in the variables below:
```
WIFI_SSID
WIFI_PASSWORD
```
Finally, go to AWS IoT Core -> Settings -> Device data endpoint -> copy the Endpoint and give its value to 'AWS_IOT_ENDPOINT'.


## AWS Workflow
![alt text](https://github.com/Gil16/Remote-led-IoT/blob/main/workflow.png?raw=true)
 
Until now we set the connection between AWS IoT Core and the ESP32, lets set up a website using AWS S3 bucket.
AWS Services -> S3 -> Create Bucket -> give it a name and keep the defaults.
Enter the bucket you just created and Upload the website code: iot_led.html

In order to trigger an action from S3 website button, we need to use AWS Service Lambda function.
Two functions are needed, one to retreive the state of the thing, and the other to update its state.

AWS Services -> Lambda Console -> Create Function -> name them as the code below, choose Python 3.8 Runtime and Create.
Replace the code with the relevant python code, e.g: Shadow_Update, Shadow_Status_Check, and **Deploy**.

The Lambdas need permission to access IoT Core thing (ESP32_LED), in order to give them the needed permission, we need to make a role.
AWS Services -> IAM -> Role -> Create Role -> AWS service, Lambda -> In permissions policies add: "AWSIoTDataAccess" -> fill the name and create.
Go to each Lambda function -> Configuration -> Permissions -> attach the role we just created.


## Shadow
As we have seen before, there are things called Shadows. They reflect the state of the device and we can use them also to change the device state using the Lambda functions.
Shadow document is a JSON document consists of 3 sections: Desired, Reported and Delta.

In our main.ino code we have those defines:
```
#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/ESP32_LED/shadow/update"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/ESP32_LED/shadow/update/delta"
```
It means we use the Shadow Public and Subscribe in order to publish the current state of the device and subscribe to change its state.

An example Shadow document:
```
{
  "desired": {
    "status": "off"
  },
  "reported": {
    "status": "on"
  },
  "delta": {
    "status": "off"
  }
}
```

**REMEMBER: Check your region.**


## API Gateway
After building all the parts, we need to connect them togather, we can do that with AWS service - API Gateway.
Invoking the led light change from the website button causing the API Gateway to invoke the Lambda to change the Shadow state in AWS IoT Core which changes the ESP32 itself and vice versa.

We showed how to access S3->IoT Core, lets see the other side: IoT Core->S3.
A Role for accessing S3 website is needed:
AWS Services -> IAM -> Role -> Create Role -> AWS service, Use case: S3, next -> Create a policy and attach it to the role, in the policy permissions add the below:
```
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "VisualEditor0",
            "Effect": "Allow",
            "Action": "s3:GetObject",
            "Resource": "arn:aws:s3:::<Your Bucket name>/iot_led.html"
        }
    ]
}
```
Click next -> give the role a name, in the Select Trusted Entities paste the following and click Create Role:
```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "",
      "Effect": "Allow",
      "Principal": {
        "Service": "apigateway.amazonaws.com"
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
```

Configuring API Gateway:
AWS Services -> Amazon API Gateway -> Create API -> REST API, Build -> name your API and keep the defaults, Create.
Enter the API you created, under the root resource create a GET method, choose AWS Service as Integrations Type, the same region as the bucket, 
S3 as AWS Service, HTTP method as GET, path override: <Your Bucket name>/iot_led.html, Execution role: copy the ARN of the role we created earlier and in Actions press **Deploy API**.
Last, go to Settings and in Binary Media Types enter: 'text/html', save and deploy.

Now we can access our S3 website.
Lets create another API which invokes Shadows.
After creating the API, create a child resource 'shadow-state' (make sure CORS is enabled), create GET and POST methods under 'shadow-state', 
type Lambda Function, Use proxy, same region, GET function is: Shadow_Status_Check and POST is Shadow_Update.
Deploy API.


## Wix Integration
Create a Wix account and Create New Site -> Edit Site -> Add -> Embed Code -> paste the presigned URL of the S3 bucket.



